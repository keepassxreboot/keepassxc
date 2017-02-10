## Using D-BUS feature

* Open keepassxc database: without password and key file

    qdbus org.keepassxc.MainWindow /keepassxc org.keepassxc.MainWindow.openDatabase /path/to/database.kdbx

* Open keepassxc database: with password but without key file

    qdbus org.keepassxc.MainWindow /keepassxc org.keepassxc.MainWindow.openDatabase /path/to/database.kdbx passwd

* Open keepassxc database: with password and key file

    qdbus org.keepassxc.MainWindow /keepassxc org.keepassxc.MainWindow.openDatabase /path/to/database.kdbx passwd /path/to/key

*  Close all keepassxc databases

    qdbus org.keepassxc.MainWindow /keepassxc org.keepassxc.MainWindow.closeAllDatabases
    
*  Exit keepassxc

    qdbus org.keepassxc.MainWindow /keepassxc org.keepassxc.MainWindow.exit

## Develop

* Regenerate XML file for DBus ( If MainWindow class public methods were modified )

    cd src/gui
    
    qdbusxml2cpp -c MainWindowAdaptor -a MainWindowAdaptor.h:MainWindowAdaptor.cpp org.keepassxc.MainWindow.xml

*  It can be usefull to know how to generate the XML adaptor

* * Generate template from sources

    qdbuscpp2xml -M -s MainWindow.h -o org.keepassxc.MainWindow.xml
    
* * Make sure interface name is org.keepassxc.MainWindow

    <interface name="org.keepassxc.MainWindow">

