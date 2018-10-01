# Quick Start for KeePassXC

This procedure gets KeePassXC running on your computer with browser integration, 
using the pre-built binaries available for [download](https://keepassxc.org/download) 
from [KeePassXC site](https://keepassxc.org).

**TL;DR** KeePassXC saves your passwords securely.
When you double-click a URL in KeePassXC, it launches your default browser to that URL.
With browser integration configured, KeePassXC automatically enters 
username/password credentials into web page fields.

## Installing and Starting KeePassXC

* [Download the native installer](https://keepassxc.org/download) and install 
KeePassXC for your  Windows, macOS, or Linux computer in the usual way for your platform.
* Open the KeePassXC application.
* Create a new database and give it a master key that's used to unlock the database file. 
This database holds entries (usernames, passwords, account numbers, notes) 
for all your websites, programs, etc.
* Create a few entries - enter the username, password, URL, and optionally notes about the entry.
* KeePassXC securely stores those entries in the database.


## Setting up Browser Integration with KeePassXC

* *Within KeePassXC*, go to **Tools->Settings** (on macOS, go to **KeePassXC->Preferences**.) 
* In **Browser Integration**, check **Enable KeePassXC browser integration**
* Right below that, click the checkbox for the browser(s) you use
Leave the other options at their defaults.
* *In your default web browser,* install the KeePassXC Browser extension/add-on. Instructions for [Firefox](https://addons.mozilla.org/en-US/firefox/addon/keepassxc-browser/) or [Chrome](https://chrome.google.com/webstore/detail/keepassxc-browser/oboonakemofpalcgghocfoadofidjkkk)
* Click the KeePassXC icon in the upper-right corner. You'll see the dialog below. 
* Click the blue Connect button to make the browser extension connect to the KeePassXC application. 
<img src="./KeePassHTTP/KeePassXC-Connect.png" height="200" alt="KeePassXC Connect dialog">

* *Switch back to KeePassXC.* You'll see a dialog (below) indicating that a request to connect has arrived. 
* Give the connection a name (perhaps *Keepass-Browsername*, any unique name will suffice) and click OK to accept it.
* This one-time operation connects KeePassXC and your browser.
<img src="./KeePassHTTP/KeePassXC-Accept-Button.png" height="200" alt="KeePassXC accept connection dialog">

## Using Browser Integration

* *Within KeePassXC,* double-click the URL of an entry,
or select it and type Ctrl+U (Cmd+U on macOS).
* Your browser opens to that URL.
* If there are username/password fields on that page, you will see the dialog below.
Click *Allow* to confirm that KeePassXC may access the credentials to auto-fill the fields. 
* Check *Remember this decision* to allow this each time you visit the page.
<img src="./KeePassHTTP/KeePassXC-Confirm.png" height="200" alt="KeePassCX Confirm Access dialog">

## Using Sharing

Sharing allows you to share a subset of your credentials with others and vice versa.

### Enable Sharing

To use sharing, you need to enable it on a database.

1. Go to Database &rarr; Database Settings
1. Check _Allow import_ if you want to import shared credentials
1. Check _Allow export_ if you want to share credentials

<img src="./KeeShare/Database-Settings.png" height="600" width="800" alt="KeePassXC Databse Sharing Settings">

### Sharing Credentials

If you checked _Allow export_ in the Sharing settings you now are good to go to share some passwords with others. Sharing always is defined on a group. If you enable sharing on a group, every entry under this group or it's children is shared. If you enable sharing on the root node, **every password** inside your database gets shared!

1. Open the edit sheet on a group you want to share
1. Select the sharing section
1. Choose _Export to path_ as the sharing method
1. Choose a path to store the shared credentials to
1. Generate a password for this share database

The export file will not be generated automatically. Instead, each time the database is saved, the file gets written. If an old file is present, the old file will be overwritten! The file should be written to a location that is accessible by others. An easy setup is a network share or storing the file inside the cloud.

<img src="./KeeShare/Share-Group.png" height="600" width="800" alt="KeePassXC Group Sharing Settings">

### Using Shared Credentials

Checking _Allow import_ in the Sharing settings of the database enables you to receive credentials from others. KeePass will watch sharing sources and import any changes immoderately into your database using the synchronization feature.

1. Create a group for import
1. Open the edit sheet on that group
1. Select the sharing section
1. Choose _Import from path_ as the sharing method
1. Choose a database that is shared with you
1. Enter the password for the shared database

<img src="./KeeShare/Import-Group.png" height="600" width="800" alt="KeePassXC Group Import Settings">

### Using Synchronized Credentials

Instead of using different groups for sharing and importing you can use a single group that acts as both. This way you can synchronize a number of credentials easily across many users without a lot of hassle.

1. Open the edit sheet on a group you want to synchronize
1. Select the sharing section
1. Choose _Synchronize with path_ as the sharing method
1. Choose a database that you want to use a synchronization file
1. Enter the password for the database

<img src="./KeeShare/Synchronize-Group.png" height="600" width="800" alt="KeePassXC Group Synchronization Settings">

## Technical Details of Sharing
Sharing relies on the combination of file exports and imports as well as the synchronization mechanism provided by KeePassXC
