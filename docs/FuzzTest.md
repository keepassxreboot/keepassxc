# Fuzz-Testing KeePassXC

Fuzz-testing = feeding random input into a program until it crashes. Be smart about what's "random" by looking at how the program executes the input.

We use the "American Fuzzy Lop" (AFL) fuzz tester (https://lcamtuf.coredump.cx/afl/).

The following assumes that all tools and libraries required to build KeePassXC from source have already been installed.

## Installing AFL

    $ sudo apt install afl

Optionally, build AFL from source:

    $ git clone https://github.com/google/AFL
    $ cd AFL
    $ make
    $ make install

## Building KeePassXC For Fuzzing

A special "instrumented build" is used that allows the fuzzer to look into the program as it executes. We place it in its own build directory so it doesn't confused with the production build.

    $ cd your_keepassxc_source_directory
    $ mkdir buildafl
    $ cd buildafl
    $ CXX=afl-g++ AFL_HARDEN=1 cmake -DWITH_XC_ALL=ON ..
    $ make

In the source code, special behavior for fuzz testing can be implemented with `#ifdef __AFL_COMPILER`. For example, in fuzz builds, the KeePassXC CLI takes the database password from environment variable `KEYPASSXC_AFL_PASSWORD` to allow non-interactive operation.

## Prepare Fuzzer Input

To get the fuzzer started, we provide empty password database files (the password is `secret`).

    $ cd buildafl
    $ mkdir -p findings/testcases
    $ cp ../share/empty*.kdbx findings/testcases

The fuzzer works by running KeePassXC with variations of this input, mutated in ways that make the program crash or hang.

## Run The Fuzzer

    $ cd buildafl
    $ KEYPASSXC_AFL_PASSWORD=secret afl-fuzz -i findings/testcases -o findings -m 2000 -t 1000 src/cli/keepassxc-cli ls @@

This fuzz-tests the `ls` command of the KeePassXC CLI, which loads and decrypts a database file and then lists its contents. The parameters mean:

* `KEYPASSXC_AFL_PASSWORD=secret`: In fuzz test builds, the KeePassXC CLI takes the database password from this environment variable.
* `-i findings/testcases`: The directory which contains the initial fuzzer input.
* `-o findings`: The directory in which to store fuzzer results.
* `-m 2000`: Fuzzer memory (in megabytes). Adjust as required if the fuzzer fails to start up.
* `-t 1000`: Timeout until a hang is detected (in milliseconds).
* `src/cli/keepassxc-cli`: The instrumented executable.
* `ls`: The subcommand we're testing.
* `@@`: The fuzzer replaces this by the name of a file with the generated input.

You may also need `export AFL_SKIP_CPUFREQ=1`.

If KeePassXC crashes or hangs when processing the input, the fuzzer writes the database file (that was used in place of `@@`) to the `findings/crashes` or `findings/hangs` directory, respectively.

To continue where the fuzzer left off, use `-i -`. To start over, remove and re-create the `findings` directory.

## More Information

AFL documentation: https://afl-1.readthedocs.io/en/latest/

Read this if you want to get serious about fuzz-testing.
