# Heart Monitor

This project builds the main Heart Monitor application from
`${WORKSPACE_ROOT}/src/main.c`.

The application uses the local `zephyr_circular_buffer` module to buffer heart
rate samples between a producer thread and a consumer thread.

## How It Works

The app has two threads:

- A producer thread generates a pseudo-random sample every second and pushes it
  into the circular buffer.
- A consumer thread wakes every 10 seconds, drains the buffered samples, and
  computes an exponential moving average (EMA).

The buffer is protected with a Zephyr mutex so both threads do not access it at
the same time.

## Project Configuration

The main application configuration is in `${WORKSPACE_ROOT}/prj.conf`.

### Very important
Please make sure that you called submodules by cloning this repo.
```
git clone --recursive https://github.com/gurkanD3/heart_monitor.git

```

or you can call it after cloning this repo
```
git clone https://github.com/gurkanD3/zephyr_circular_buffer.git

```

Important options:

- `CONFIG_CIRCULAR_BUFFER=y`
- `CONFIG_STATIC_ARRAY=y`
- `CONFIG_CIRCULAR_BUFFER_SIZE=30`
- `CONFIG_USE_SEGGER_RTT=y`
- `CONFIG_RTT_CONSOLE=y`

## Build with Dockerfile
inside of project folder: `Image size 9.8GB`
```
docker build -t hear_monitor_nrf .
docker run --rm -v "$(pwd)/out:/work/Heart_monitor/out" heart-monitor-nrf
```

During run, first it will initalize unit tests of circular buffer. Then it will generate bin files 
Inside of out folder, binaries necessary to flashing.


## Build With nRF Connect For VS Code

These steps assume:

- nRF Connect for VS Code is installed
- an nRF Connect SDK toolchain is installed
- this repository is already opened in VS Code

## Open The Application

In VS Code:

1. Open the nRF Connect extension.
2. Choose `Open an existing application`.
3. Select the repository root: `${WORKSPACE_ROOT}`

This project uses `${WORKSPACE_ROOT}/CMakeLists.txt` to build the app from
`${WORKSPACE_ROOT}/src/main.c`.

## Add A Build Configuration

1. Click `Add Build Configuration`.
2. Select your board, for example `nrf52833dk/nrf52833`.
3. Use the default `${WORKSPACE_ROOT}/prj.conf`.
4. Start the build.

## Flash And Monitor

1. Connect the board over USB.
2. Click `Flash`.
3. Open the RTT terminal or log viewer in nRF Connect for VS Code.

The app is configured to log over RTT.

## Expected Runtime Behavior

- A new sample is generated about once per second.
- Buffered samples are consumed about every 10 seconds.
- The log shows consumed samples and the current EMA value.
- If the buffer fills before the consumer drains it, push operations can fail
  with a full-buffer error.

## Circular Buffer Module

The circular buffer implementation is included from
`${WORKSPACE_ROOT}/zephyr_circular_buffer`.

Users of this repository do not need to open the module examples to run the
main Heart Monitor app. Those example applications are only additional
reference material for the module itself.
