# AI Assistance Disclosure

This experimental Qt 6 port was developed with substantial assistance from OpenAI Codex using a GPT-5-based model.
The exact backend model snapshot was not exposed by the tool.

## Provenance and scope

The design and implementation build on Jonathan White's upstream `feature/remember-quickunlock` branch, especially
commit `d2ad2a95` ("Add support to remember quick unlock on Windows and macOS"), and on the current KeePassXC Qt 6
`develop` branch. The AI agent inspected those revisions and the local repository and was used to:

* select and port the Windows-specific subset of the upstream work;
* implement the opt-in configuration, UI lifecycle, Windows Password Vault persistence, and failure handling;
* identify and prevent persistence of non-opted session keys after an abnormal process exit;
* add a configuration regression test and technical documentation;
* configure, format, build, test, and review the local changes; and
* analyze security boundaries and upstream integration risks.

The new and modified code in this branch is predominantly AI-generated or AI-modified. Human prompts supplied the
goal, constraints, corrections, and acceptance decisions. AI assistance must not be treated as a substitute for
maintainer review, a security audit, or independent Windows testing.

## Data and review responsibility

No real database passwords, production databases, private keys, key files, recovery material, or other intentional
secrets were supplied to the model. Local source files, Git history, compiler and test output, and local toolchain
paths were available to the agent.

During diagnosis, the full isolated manual-test configuration was printed to AI-visible tool output. It contained
an automatically generated KeeShare private key belonging only to that disposable test configuration. It was not a
production key and was not associated with a real shared database, but it must be treated as disclosed test material.
The isolated configuration is to be deleted after testing and must not be reused.

Every submitted line must be reviewed by the human contributor. In particular, reviewers should independently
verify the Windows Hello key lifecycle, Password Vault isolation and limits, record replacement and deletion,
credential-change invalidation, memory handling, cancellation behavior, and compatibility with supported Windows
and SDK versions. If the patch is split, rewritten, or receives further AI assistance, the pull-request disclosure
must be updated accordingly.
