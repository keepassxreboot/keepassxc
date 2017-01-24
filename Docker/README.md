# How to build the Docker image?
To build the Docker image open a terminal in this directory and run the command: `docker build -t keepassxc[:tag]`. The optional tag may contain the version or the actual commit number.

# How to run `keepassxc`?
The following command:

    docker run --rm -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /etc/localtime:/etc/localtime -v /host/path/to/db-directory:/db keepassxc

Will run `keepassxc`. The directory on the host named `/host/path/to/db-directory` will be mounted in the directory `/db` of the container. This should be used to keep a persistent database.
