# Contributing to KeePassXC

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to KeePassXC on GitHub.
These are just guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

#### Table of contents

[What should I know before I get started?](#what-should-i-know-before-i-get-started)
  * [Open Source Contribution Policy](#open-source-contribution-policy)

[How can I contribute?](#how-can-i-contribute)
  * [Feature requests](#feature-requests)
  * [Bug reports](#bug-reports)
  * [Discuss with the team](#discuss-with-the-team)
  * [Your first code contribution](#your-first-code-contribution)
  * [Pull requests](#pull-requests)
  * [Translations](#translations)

[Styleguides](#styleguides)
  * [Git branch strategy](#git-branch-strategy)
  * [Git commit messages](#git-commit-messages)
  * [Coding styleguide](#coding-styleguide)


## What should I know before I get started?
### Open Source Contribution Policy
**Source**: [Version 0.3, 2015–11–18](https://medium.com/@jmaynard/a-contribution-policy-for-open-source-that-works-bfc4600c9d83#.i9ntbhmad)

#### Policy

We will accept contributions of good code that we can use from anyone.

#### What this means

 - “We will accept”: This means that we will incorporate your contribution into the project’s codebase, adapt it as needed, and give you full credit for your work.
 - “contributions”: This means just about anything you wish to contribute to the project, as long as it is good code we can use. The easier you make it for us to accept your contribution, the happier we are, but if it’s good enough, we will do a reasonable amount of work to use it.
 - “of good code”: This means that we will accept contributions that work well and efficiently, that fit in with the goals of the project, that match the project’s coding style, and that do not impose an undue maintenance workload on us going forward. This does not mean just program code, either, but documentation and artistic works as appropriate to the project.
 - “that we can use”: This means that your contribution must be given freely and irrevocably, that you must have the right to contribute it for our unrestricted use, and that your contribution is made under a license that is compatible with the license the project has chosen and that permits us to include, distribute, and modify your work without restriction.
 - “from anyone”: This means exactly that. We don’t care about anything but your code. We don’t care about your race, religion, national origin, biological gender, perceived gender, sexual orientation, lifestyle, political viewpoint, or anything extraneous like that. We will neither reject your contribution nor grant it preferential treatment on any basis except the code itself. We do, however, reserve the right to tell you to go away if you behave too obnoxiously toward us.

#### If Your Contribution Is Rejected

If we reject your contribution, it means only that we do not consider it suitable for our project in the form it was submitted. It is not personal. If you ask civilly, we will be happy to discuss it with you and help you understand why it was rejected, and if possible improve it so we can accept it.

#### Revision History
 * 0.1, 2011–11–18: Initial draft.
 * 0.2, 2011–11–18: Added “If Your Contribution Is Rejected” section.
 * 0.3, 2011–11–19: Added “irrevocably” to “we can use” and changed “it” to “your contribution” in the “if rejected” section. Thanks to Patrick Maupin.


## How can I contribute?
### Feature requests

We're always looking for suggestions to improve our application. If you have a suggestion to improve an existing feature, or would like to suggest a completely new feature for KeePassXC, please use the [issue tracker on GitHub][issues-section].

### Bug reports

Our software isn't always perfect, but we strive to always improve our work. You may file bug reports in the issue tracker.

Before submitting a bug report, check if the problem has already been reported. Please refrain from opening a duplicate issue. If you want to add further information to an existing issue, simply add a comment on that issue.

### Discuss with the team

As with feature requests, you can talk to the KeePassXC team about bugs, new features, other issues and pull requests on the dedicated issue tracker, or in the IRC channel on Freenode (`#keepassxc-dev` on `irc.freenode.net`, or use a [webchat link](https://webchat.freenode.net/?channels=%23keepassxc-dev)).

### Your first code contribution

Unsure where to begin contributing to KeePassXC? You can start by looking through these `beginner` and `help-wanted` issues:

* [Beginner issues][beginner] – issues which should only require a few lines of code, and a test or two.
* ['Help wanted' issues][help-wanted] – issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, looking at the number of comments on an issue can give a general idea of how much an impact a given change will have.

### Pull requests

Along with our desire to hear your feedback and suggestions, we're also interested in accepting direct assistance in the form of code.

All pull requests must comply with the above requirements and with the [styleguides](#styleguides).

### Translations

Translations are managed on [Transifex](https://www.transifex.com/keepassxc/keepassxc/) which offers a web interface.
Please join an existing language team or request a new one if there is none.

## Styleguides

### Git branch strategy

The Branch Strategy is based on [git-flow-lite](http://nvie.com/posts/a-successful-git-branching-model/).

* **master** – points to the latest public release
* **develop** – points to the development of the next release, contains tested and reviewed code
* **feature/**[name] – points to a branch with a new feature, one which is candidate for merge into develop (subject to rebase)
* **hotfix/**[id]-[description] – points to a branch with a fix for a particular issue ID


### Git commit messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to…" not "Moves cursor to…")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally
* If your pull request fixes an existing issue, add "…, resolves #ISSUENUMBER" to your main commit
* When only changing documentation, include `[ci skip]` in the commit description

### Coding styleguide

The coding style of the project is enforced using llvm's `clang-format` formatting tool. A thorough description
of the coding style can be found in the `.clang-format` file, but the main conventions are presented here.

Formatting can be performed automatically by calling `make format` from the `build/` directory.

Note that [formatting can be disabled on a piece of code](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#disabling-formatting-on-a-piece-of-code) if manual formatting is deemed more readable.

#### Naming convention
`lowerCamelCase`

For names made of only one word, the first letter should be lowercase.
For names made of multiple concatenated words, the first letter of the whole is lowercase, and the first letter of each subsequent word is capitalized.

#### Indention
For **C++ files** (*.cpp .h*): 4 spaces  
For **Qt-UI files** (*.ui*): 2 spaces

#### Pointers
```c
int* count;
```

#### Braces
```c
if (condition) {
    doSomething();
}

void ExampleClass::exampleFunction()
{
    doSomething();
}
```

#### Switch statement
```c
switch (a) {
case 1:
    doSomething();
    break;

default:
    doSomethingElse();
    break;
}
```

#### Member variables
Use prefix: `m_*`

Example: `m_variable`

#### GUI widget names
Widget names must be related to the desired program behavior, and preferably end with the widget's classname.

Example: `<widget class="QCheckBox" name="rememberCheckBox">`



[beginner]:https://github.com/keepassxreboot/keepassx/issues?q=is%3Aopen+is%3Aissue+label%3Abeginner+label%3A%22help+wanted%22+sort%3Acomments-desc
[help-wanted]:https://github.com/keepassxreboot/keepassx/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22+sort%3Acomments-desc
[issues-section]:https://github.com/keepassxreboot/keepassxc/issues
