# AI Assistance Disclosure

This experimental persistent Windows Hello unlock prototype was developed with substantial assistance from
OpenAI Codex using a GPT-5-based model. The exact backend model snapshot was not exposed by the tool.

## Scope of assistance

The AI agent had access to the local KeePassXC source tree and was used to:

* inspect the existing Quick Unlock, `CompositeKey`, database-open, and database-key-change code paths;
* propose and implement the prototype architecture and code changes;
* write and revise automated tests and technical documentation;
* run local configuration, build, formatting, and test commands; and
* review the resulting changes for security boundaries, failure behavior, and upstream suitability.

The resulting patch is predominantly AI-generated or AI-modified. Human prompts supplied the feature goal,
constraints, corrections, and acceptance decisions. A human operator also performed the reported Windows build and
test runs. AI assistance must not be treated as a substitute for maintainer review, a security audit, or independent
testing.

## Data and credentials

No real database passwords, production databases, private keys, key files, recovery material, or other intentional
secrets were supplied to the model. Local repository contents, compiler output, test output, and toolchain paths were
available to the agent while it worked. Testing should continue to use throwaway databases and synthetic
credentials only.

## Model limitations and review responsibility

The agent can introduce subtle implementation, cryptographic, platform-API, lifecycle, or documentation errors.
Every submitted line must therefore be reviewed by the human contributor, and the change requires the normal
KeePassXC maintainer review and security scrutiny. In particular, the Windows Hello key lifecycle, local persistence
boundary, memory handling, deletion semantics, and behavior after credential or operating-system changes remain
areas requiring independent verification on supported Windows systems.

This disclosure describes the prototype in this branch. If parts of it are rewritten, split into another pull
request, or supplemented with additional AI assistance, the pull-request disclosure should be updated accordingly.
