# Contributing to KeePassX Reboot

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to KeePassX Reboot on GitHub.
These are just guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

#### Table Of Contents

[What should I know before I get started?](#what-should-i-know-before-i-get-started)
  * [Open Source Contribution Policy](#open-source-contribution-policy)

[How Can I Contribute?](#how-can-i-contribute)
  * [Feature Requests](#feature-requests)
  * [Bug Reports](#bug-reports)
  * [Your First Code Contribution](#your-first-code-contribution)
  * [Pull Requests](#pull-requests)
  * [Translations](#translations)

[Styleguides](#styleguides)
  * [Git Branch Strategy](#git_branch_strategy)
  * [Git Commit Messages](#git-commit-messages)
  * [Coding Styleguide](#coding-styleguide)


## What should I know before I get started?
### Open Source Contribution Policy
[Version 0.3, 2015–11–18](https://medium.com/@jmaynard/a-contribution-policy-for-open-source-that-works-bfc4600c9d83#.i9ntbhmad)

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


## How Can I Contribute?
### Feature Requests

We're always looking for suggestions to improve our application. If you have a suggestion for improving an existing feature, or would like to suggest a completely new feature for KeePassX Reboot, please use the Issues section or our [Google Groups](https://groups.google.com/forum/#!forum/keepassx-reboot) forum.

### Bug Reports

Our software isn't always perfect, but we strive to always improve our work. You may file bug reports in the Issues section.

Before submitting a Bug Report, check if the problem has already been reported. Please refrain from opening a duplicate issue. If you want to highlight a deficiency on an existing issue, simply add a comment.

### Discuss with the Team

You can talk to the KeePassX Reboot Team about Bugs, new feature, Issue and PullRequests at our [Google Groups](https://groups.google.com/forum/#!forum/keepassx-reboot) forum

### Your First Code Contribution

Unsure where to begin contributing to KeePassX Reboot? You can start by looking through these `beginner` and `help-wanted` issues:

* [Beginner issues][beginner] - issues which should only require a few lines of code, and a test or two.
* [Help wanted issues][help-wanted] - issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

### Pull Requests

Along with our desire to hear your feedback and suggestions, we're also interested in accepting direct assistance in the form of code.

All pull requests must comply with the above requirements and with the [Styleguides](#styleguides).

### Translations

Translations are managed on [Transifex](https://www.transifex.com/keepassxc/keepassxc/) which offers a web interface.
Please join an existing language team or request a new one if there is none.

## Styleguides

### Git Branch Strategy

The Branch Strategy is based on [git-flow-lite](http://nvie.com/posts/a-successful-git-branching-model/).

* **master** -> always points to the last release published
* **develop** -> points to the next planned release, tested and reviewed code
* **feature/**[name] -> points to brand new feature in codebase, candidate for merge into develop (subject to rebase)


### Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally
* When only changing documentation, include `[ci skip]` in the commit description
* Consider starting the commit message with an applicable emoji:
    * :memo: `:memo:` when writing docs
    * :penguin: `:penguin:` when fixing something on Linux
    * :apple: `:apple:` when fixing something on macOS
    * :checkered_flag: `:checkered_flag:` when fixing something on Windows
    * :bug: `:bug:` when fixing a bug
    * :fire: `:fire:` when removing code or files
    * :green_heart: `:green_heart:` when fixing the CI build
    * :white_check_mark: `:white_check_mark:` when adding tests
    * :lock: `:lock:` when dealing with security


### Coding Styleguide

This project follows the [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style). All submissions are expected to follow this style.

In particular Code must follow the following specific rules:

#### Naming Convention
`lowerCamelCase`

For names made of only one word, the fist letter is lowercase.  
For names made of multiple concatenated words, the first letter is lowercase and each subsequent concatenated word is capitalized.

#### Indention
For C++ files (.cpp .h): 4 spaces  
For Qt-UI files (.ui): 2 spaces

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

#### GUI Widget names
Widget names must be related to the desired program behaviour.  
Preferably end the name with the Widget Classname

Example: `<widget class="QCheckBox" name="rememberCheckBox">`



[beginner]:https://github.com/keepassxreboot/keepassx/issues?q=is%3Aopen+is%3Aissue+label%3Abeginner+label%3A%22help+wanted%22+sort%3Acomments-desc
[help-wanted]:https://github.com/keepassxreboot/keepassx/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22+sort%3Acomments-desc
