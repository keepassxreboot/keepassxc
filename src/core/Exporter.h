#ifndef KEEPASSX_EXPORTER_H
#define KEEPASSX_EXPORTER_H

class Database;
class Group;

class Exporter
{
public:
    virtual Database* exportGroup(Group* group) = 0;
    virtual ~Exporter() {}
};

#endif // KEEPASSX_EXPORTER_H
