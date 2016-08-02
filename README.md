# Splash

Splash is a [Greenhouse](http://greenhouse.oblong.com/) program that transforms the Leap Motion's relative coordinates to absolute coordinates.  Splash streams its data output as a series of messages into a Greenhouse pool (named **leap**).  Any number of other programs can listen to that pool and get the data.

(Pools are part of the Greenhouse message-passing system known as **Plasma**.  It's a lighweight publish/subscribe system that scratches some of the same itches as OSC or 0MQ's pubsub.  More details about **Plasma** [can be found here](http://greenhouse.oblong.com/reference/messaging.html#qr_pools).)

Splash is a command-line utility -- it runs in the terminal only.

Splash reads Leap data using the Leap SDK (which you must have already installed).  Splash also assumes you have already installed [Greenhouse](http://greenhouse.oblong.com/), a free SDK from Oblong for creating spatial, gestural, multi-modal, and multi-machine applications.

To learn more about how to set up a Greenhouse program to work with Splash and Leap Motion input, see [this tutorial on the Greenhouse site](http://greenhouse.oblong.com/learning/hardware_leap.html).

## Dependencies

- g-speak platform -- a proprietary SDK provided by Oblong Industries. Contact [Oblong Industries, Inc.](mailto:solutions@oblong.com)
- [LeapSDK](https://developer.leapmotion.com/documentation/cpp/index.html)
- [CMake](https://cmake.org/)
  - Install Mac OS X using [brew](http://brew.sh/)

    $ brew install cmake

  - Install on Ubuntu

    $ sudo apt-get install cmake


## Building

Make sure to set `LEAPSDK_HOME` to the directory where your Leap SDK lives.  Then build (we use our internal build utility, [obi](https://github.com/Oblong/obi.git) which works off of cmake).

    # With obi
    $ export LEAPSDK_HOME=/path/to/LeapSDK
    $ obi build

or

    # Without obi
    $ export LEAPSDK_HOME=/path/to/LeapSDK
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

### Leap SDK version

To support the [`POLICY_BACKGROUND_FRAMES`](https://developer.leapmotion.com/articles/testing-background-leap-applications)
flag in the Leap Motion controller, `splash` requires at least
version 0.7.9 of the Leap SDK.


## Running

### Before running splash

1. Connect a Leap device
2. Verify that the Leap system is working (e.g. with the Leap's own software).

### Actually running splash

The Leap shared library must be on your `LD_LIBRARY_PATH` (or `DYLD_LIBRARY_PATH` if you're using a Mac) before you start splash.  e.g.:

    # With obi
    $ export DYLD_LIBRARY_PATH=/path/to/LeapSDK/lib
    $ obi go

or

    # Without obi
    $ DYLD_LIBRARY_PATH=/path/to/LeapSDK/lib
    $ build/splash

### To check that it's working

To see the output from `splash` in a human-readable format, use the peek command at the terminal: `peek leap`.  You should see data scrolling by quickly, representing the messages that splash is depositing into the **leap** pool; when you hover a hand in view of the Leap, the data should resemble the output shown below in the **Output Format** section.


### Arguments

Optionally, `splash` takes the fully qualified path to a simple settings file that describes the physical location of the Leap sensor.  The file format is as follows:

    %YAML 1.1
    %TAG ! tag:oblong.com,2009:slaw/
    --- !protein
    descrips: # ignored
    ingests:
      leap:
        cent: [0, 0, 0]   # required, center of the leap device in absolute coordinates
        norm: [0, 1, 0]   # required, normal vector from the face of the leap device
        over: [1, 0, 0]   # required, vector pointing lengthwise along the device
        provenance: "leap-whatever"  # optional, defaults to "leap-$HOSTNAME"

'norm' is usually 'up' along the Y axis, because the Leap device is usually lying on a flat surface.

'over' is usually to the user's right (so it's positive in the X axis).

If a settings file is not supplied on the command line, `splash` will try to find a 'leap' section in the ingests of the `/etc/oblong/screen.protein` file.  (This file would have been installed by the Greenhouse installer.)

If a 'leap' section isn't found in screen.protein, `splash` will just assume that your Leap is 500mm in front and 200mm below the center of the "main" screen described in your `/etc/oblong/screen.protein` file.

If a `screen.protein` file can't be found, or if it does not contain a screen called "main", `splash` will warn you of as much and will pass the leap's data through in the native (relative) coordinates supplied by the Leap SDK.

For more information on how Greenhouse works with 3D space refer to this [Spatial Considerations tutorial](http://greenhouse.oblong.com/learning/spatial.html).

### Environment

By default, `splash` deposits its output (a series of messages which in Oblong parlance are referred to as *proteins*') into a pool called "leap".  Any number of Greenhouse programs on the same machine can listen to (or "particpate in") that pool.

If there are Greenhouse programs on a different machine that would like to listen, they can refer to the pool as "tcp://your-host-name/leap".

You can optionally specify a different pool to use via the `LEAP_POOL` environment variable.
Note that this pool must exist before you run `splash.`

    $ p-create my_new_leap_pool
    $ LEAP_POOL=my_new_leap_pool ./splash


## Output Format

To see the output from `splash` in a human-readable format, use the peek command.
`peek leap` (or the name of some other pool you specified) at the command line.

`splash` produces proteins of the following format:

    descrips:
    - greenhouse
    - leap
    - 0.7.9 # The version of the Leap Motion API
    ingests:
      leap:
        orig: v3float64 # the center of the Leap device itself
        norm: v3float64 # the leap's normal vector, usually [0,1,0]
        over: v3float64 # the direction to the leap's right, usually [1,0,0]
        prov: Str # the provenance associated with the leap
      frame:
        ts: int64 # Frame timestamp
        id: int64 # Frame ID
        hands: # a list of hands
        - id: int64
          dir: v3float64 # the direction the hand is pointing in
          plmnrm: v3float64 # palm normal vector
          plmpos: v3float64 # palm position
          plmvel: v3float64 # palm velocity
          center: v3float64 # Center of a sphere fit to the hand
          radius: float64 # radius of a sphere fit to the hand
          orig: v3float64 # palm position, ob-named
          thru: v3float64 # orig + dir
        pntrs: # a list of pointers (fingers, tools, etc.)
        - id: int64
          dir: v3float64 # the direction the pointer is pointing in
          hand: int64 # the ID of the hand the pointer is associated with
          isfngr: bool # Does the leap believe that this pointer is a finger?
          istool: bool # Does the leap believe that this pointer is a tool?
          length: float64 # the length of the pointer
          t-pos: v3float64 # Tip position
          t-vel: v3float64 # Tip velocity
          width: float64 # Width of the pointer
          orig: v3float64 # tip position again, ob-named
          thru: v3float64 # orig + dir * length
        gests: # A list of gestures
        - id: int64
          dur: int64 # How long has the gesture been active, microseconds.
          dursec: float64 # How long has the gesture been active, seconds.
          hands: # a list of hand ids associated with the gesture
          - int64
          - int64
          pntrs: # a list of pointer ids associated with the gesture
          - int64
          - int64
          state: Str # one of "start", "update", "stop" or "invalid"
          type: Str # one of "circle", "swipe", "s-tap", "k-tap" or "invalid"
          point: int64 # Id of pointer, only for circle, swipe, s-tap or k-tap
          ## The following occur in a "circle" gesture
          cent: v3float64 # Center of a "circle" gesture
          norm: v3float64 # Normal of a "circle" gesture
          prog: float64 # The number of times a finger has traversed a "circle"
          radius: float64 # The radius of a "circle" gesture
          ## The following occur in a "swipe" gesture
          dir: v3float64 # Direction of the swipe
          pos: v3float64 # Current position of the swipe
          speed: float64 # Speed of the swipe in mm/second
          start: v3float64 # The starting position of the swipe
          ## The following occur in s-tap (screen tap) and k-tap (key tap) gestures
          dir: v3float64 # Direction of the tap
          pos: v3float64 # Position of the tap
          prog: float64 # Always 1.0, for whatever reason

