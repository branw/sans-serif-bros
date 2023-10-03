# Sans Serif Bros.

A classic ASCII platformer revived with a Telnet interface

![Screenshot](https://user-images.githubusercontent.com/2104778/84941147-f33ed200-b0ae-11ea-9bb1-4fc491022c53.png)

## Progress

- Game
  - [x] Implement original game logic
  - [x] Recover and import original default levels
  - [ ] Add interface for exploring the default levels
  - [ ] Add additional info to game HUD
- Telnet
  - [x] Implement basic spec
  - [x] Create framework for terminal input processing and output rendering
  - [x] Support Telnet window size options (RFC 1073)
  - [ ] Support user persistence (e.g. keeping statistics between sessions)
- Level Pit
  - [x] Recover and import original user-submitted levels
  - [x] Track level attempt statistics
  - [ ] Add level attempt playback
  - [ ] Add level creator
- Internals
  - [x] Support single-player and multi-player configurations
  - [x] Support Linux builds
  - [x] Support macOS builds
  - [ ] Support Windows builds
  - [x] Use SQLite as a data store

## Building

`ssb` requires CMake, a C11 compiler, *nix headers, and SQLite (optionally
installed with the included vcpkg manifest).

To build and run unit tests:

```shell script
cmake .;
make;
ctest --verbose;
```

## Playing

`ssb` can ran as either a multi-player Telnet server, or as a single-player
terminal game.

Both modes require a database and optionally, a folder of existing levels to
import. The path to the database file can be specified using
`-d path/to/db.sqlite` (defaulting to `./ssb.sqlite` otherwise), and the path
to the levels folder can be specified using `-l path/to/levels/` (defaulting
to `./levels/`).

### As a Telnet Server (Multi-Player)

`ssb` is designed to run as a Telnet server, allowing multiple simultaneous
players and player-submitted level designs.

- To run the server:
  ```shell script
  ./ssb
  ```
  
- To connect as a client locally:
  ```shell script
  telnet 127.0.0.1
  ```

#### Port Choice

Traditionally, Telnet servers are exposed on port 23. Linux, however, prevents
non-superuser access to privileged ports, i.e. ports below 1024. You can either:

- Run `ssb` as root (bad idea):
  ```shell script
   sudo ./ssb
  ```

- Give the `ssb` binary the `CAP_NET_BIND_SERVICE` capability (better idea):
  ```shell script
   sudo setcap CAP_NET_BIND_SERVICE=+eip ./ssb
  ```
  
- Run `ssb` on a higher port:
  ```shell script
  ./ssb -p 8080
  ```

### As a Terminal Game (Single-Player)

`ssb` can also be ran serverless via standalone mode:

```shell script
./ssb -s
```

## History

"[Super Serif Brothers](https://foon.uk/farcade/ssb/)" was a web platformer
created by [Robin Allen](https://foon.uk/) in the early days of the internet.
This is a faithful recreation that takes the ASCII game one step further and
brings it into the terminal.

The original game included fourteen 80x25 character levels, plus a "level pit"
where netizens could share their own level designs -- some wonderful works of
art that pushed the format to its limits, among other levels that serve as more
of "digital graffiti." Unfortunately a good chunk of these player-submitted
levels have been lost to time. Those that could be recovered are included,
along with the original fourteen levels, in the `levels` directory.

## License

`ssb` is released under the MIT License. See `LICENSE` for more information.
