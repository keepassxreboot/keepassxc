const fs = require('node:fs');
const { setTimeout: sleep } = require('node:timers/promises');

const REPOSITORY = 'keepassxreboot/keepassxc';
const GITHUB_API = 'https://api.github.com';
const TEAMCITY_API = 'https://ci.keepassxc.org/app/rest';
const INTERVAL_MS = 20_000;
const POLL_COUNT = 25;

async function requestJson(url, token, options = {}) {
    const response = await fetch(url, {
        ...options,
        headers: {
            Accept: 'application/json',
            Authorization: `Bearer ${token}`,
            ...options.headers,
        },
        redirect: 'error',
        signal: AbortSignal.timeout(30_000),
    });
    if (!response.ok) {
        throw new Error(`${options.method || 'GET'} ${url.pathname} failed: HTTP ${response.status}`);
    }
    return response.json();
}

async function findPullRequests(run, token, request = requestJson) {
    const repository = run.head_repository?.full_name;
    const branch = run.head_branch;
    if (!repository || !branch || !/^[0-9a-f]{40}$/i.test(run.head_sha)) {
        throw new Error('Workflow run is missing its head repository, branch, or SHA');
    }

    const matches = [];
    const url = new URL(`/repos/${REPOSITORY}/pulls`, GITHUB_API);
    url.search = new URLSearchParams({
        state: 'open',
        head: `${repository.split('/')[0]}:${branch}`,
        per_page: '100',
    });
    for (let page = 1; ; page++) {
        url.searchParams.set('page', page);
        const pulls = await request(url, token);
        matches.push(...pulls
            .filter((pull) => pull.head?.repo?.full_name === repository
                && pull.head.sha === run.head_sha && pull.base?.repo?.full_name === REPOSITORY)
            .map((pull) => pull.number));
        if (pulls.length < 100) {
            return matches;
        }
    }
}

function isMatchingBuild(build, pullNumbers, sha) {
    return Number.isSafeInteger(build.id)
        && build.approvalInfo?.status === 'waitingForApproval'
        && pullNumbers.some((number) =>
            build.branchName === `pull/${number}`
            && build.approvalInfo.approvalReasons?.some((reason) =>
                reason.url === `https://github.com/${REPOSITORY}/pull/${number}`)
            && build.revisions?.revision?.some((revision) =>
                revision.vcsBranchName === `refs/pull/${number}/head` && revision.version === sha));
}

async function queuedBuilds(token, request = requestJson) {
    const builds = [];
    const count = 100;
    const url = new URL(`${TEAMCITY_API}/buildQueue`);
    url.searchParams.set('fields', 'count,build(id,buildTypeId,branchName,revisions,approvalInfo)');
    for (let start = 0; ; start += count) {
        url.searchParams.set('locator', `project:KeePassXC,count:${count},start:${start}`);
        const page = await request(url, token);
        if (!Number.isInteger(page.count) || page.count < 0 || !Array.isArray(page.build ?? [])) {
            throw new Error('Invalid TeamCity build queue response');
        }
        const items = page.build ?? [];
        if (items.length !== page.count) {
            throw new Error('Incomplete TeamCity build queue response');
        }
        builds.push(...items);
        if (page.count < count) {
            return builds;
        }
    }
}

async function approveBuild(id, token, request = requestJson) {
    if (!Number.isSafeInteger(id)) {
        throw new Error('TeamCity build ID must be a safe integer');
    }
    if (!token) {
        throw new Error('TeamCity token must be configured');
    }
    const url = new URL(`${TEAMCITY_API}/buildQueue/id:${id}/approve`);
    url.searchParams.set('approveAll', 'false');
    const result = await request(url, token, {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: '',
    });
    if (!['approved', 'waitingForApproval'].includes(result.status)) {
        throw new Error(`Unexpected approval status for TeamCity build ${id}: ${result.status}`);
    }
    return result;
}

async function main(event, githubToken, teamcityToken, request = requestJson, wait = sleep) {
    if (!githubToken || !teamcityToken) {
        throw new Error('GH_TOKEN and TEAMCITY_APPROVAL_TOKEN must be configured');
    }
    const run = event.workflow_run;
    if (!run || run.event !== 'pull_request') {
        throw new Error('Expected a pull_request workflow run');
    }

    const workflowUrl = new URL(`/repos/${REPOSITORY}/actions/workflows/codeql.yml`, GITHUB_API);
    const workflow = await request(workflowUrl, githubToken);
    if (run.workflow_id !== workflow.id || run.path !== '.github/workflows/codeql.yml') {
        throw new Error('The triggering workflow is not the repository CodeQL workflow');
    }

    const approved = new Set();
    for (let attempt = 0; attempt < POLL_COUNT; attempt++) {
        if (attempt > 0) {
            await wait(INTERVAL_MS);
        }
        const pulls = await findPullRequests(run, githubToken, request);
        if (pulls.length === 0) {
            console.log('No open pull request still has the triggering commit; stopping.');
            return;
        }
        for (const build of await queuedBuilds(teamcityToken, request)) {
            if (approved.has(build.id) || !isMatchingBuild(build, pulls, run.head_sha)) {
                continue;
            }
            const result = await approveBuild(build.id, teamcityToken, request);
            approved.add(build.id);
            console.log(`TeamCity build ${build.id} (${build.buildTypeId}): ${result.status}`);
        }
    }
    if (!approved.size) {
        console.log('::warning::No matching TeamCity builds were awaiting approval during the polling window.');
    } else {
        console.log(`Submitted approval for ${approved.size} TeamCity build(s).`);
    }
}

if (require.main === module) {
    main(JSON.parse(fs.readFileSync(process.env.GITHUB_EVENT_PATH, 'utf8')),
        process.env.GH_TOKEN, process.env.TEAMCITY_TOKEN).catch((error) => {
        console.error(error.message);
        process.exitCode = 1;
    });
}

module.exports = { approveBuild, findPullRequests, isMatchingBuild, main, queuedBuilds };
