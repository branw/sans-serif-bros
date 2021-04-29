# Sans Serif Bros.

A classic ASCII platformer revived with a Telnet interface

![Screenshot](https://user-images.githubusercontent.com/2104778/84941147-f33ed200-b0ae-11ea-9bb1-4fc491022c53.png)

## Building

`ssb` requires `gcc`, `make`, and Linux headers. To build and run unit tests:

```shell script
make
```

## Playing

`ssb` requires a directory for storing the level database. Although write-access
is not required, it does allow for levels and metadata to be saved. This
directory defaults to `./levels`, but can be specified by the
`-d path/to/levels` option.

### As a Server

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

### Without a Server

`ssb` can also be ran serverless via standalone mode:

```shell script
./ssb -s
```

## History

"Super Serif Brothers" was released by Robin Allen in the early days of
web-based games.

The original game also featured a "level pit" in which players could design and
upload their own levels. Unfortunately, a good chunk of these designs were lost
and only a small number could be recovered from archive.org records.

## License

`ssb` is released under the MIT License. See `LICENSE` for more information.
