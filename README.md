# Splash

Splash is a [g-speak](http://platform.oblong.com/) program that transforms [Leap Motion's](https://www.leapmotion.com/) relative coordinates to absolute coordinates and converts the hand positions into g-speak's own gestural language, known as 'gripes'. Splash streams its data output as a series of messages into a pool named **gripes**.  Other g-speak programs can then connect to that pool to respond to the gestural input.

(Pools are part of the g-speak's message-passing system known as **Plasma**.  It's a lighweight publish/subscribe system that scratches some of the same itches as OSC or 0MQ's pubsub.  More details about **Plasma** [can be found here](https://platform.oblong.com/learning/g-speak/structure/libplasma/).)

Splash is a command-line utility -- it runs in the terminal only.

Splash reads Leap data using the [Leap SDK](https://developer-archive.leapmotion.com/documentation/v2/cpp/index.html) (which you must have already installed).  Splash also assumes you have already installed g-speak, an SDK from Oblong for creating spatial, gestural, multi-modal, and multi-machine applications. [Contact Us](mailto:platform-support@oblong.com) for licensing information.

To learn more about how to set up a g-speak program to work with Splash and Leap Motion input, see [this tutorial on the Platform site](TODO).

## Dependencies

- g-speak platform -- a proprietary SDK provided by Oblong Industries. Contact [Oblong Industries, Inc.](mailto:platform-support@oblong.com)
- [LeapSDK](https://developer.leapmotion.com/documentation/cpp/index.html)  
  We highly suggest using the latest legacy API: 2.3.1. We do not recommend using the Orion SDK as it assumes the leap controller is attached to your head.
  - download [here](https://developer.leapmotion.com/sdk/v2) and agree to [Leap Motion's SDK Agreement](https://central.leapmotion.com/agreements/SdkAgreement)
  - make note of where you saved your extracted folder. You will use this path later to set the LEAPSDK_HOME environment variable.
  - run the Leap Motion Installer

- [CMake](https://cmake.org/)
  - Mac OS X using [brew](http://brew.sh/)

    `$ brew install cmake`

  - Ubuntu

    `$ sudo apt-get install cmake`


## Building

Set `LEAPSDK_HOME` to the directory where your Leap SDK lives and then build. We use our internal build utility, [obi](https://github.com/Oblong/obi.git) which wraps cmake to make building and running locally or on remote machines a breeze.

  ```bash
  # With obi
  $ export LEAPSDK_HOME=/path/to/LeapSDK
  $ obi build
  ```

or

  ```bash
  # With cmake
  $ export LEAPSDK_HOME=/path/to/LeapSDK
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  ```

## Running

### Before running splash

1. Connect a Leap device
2. Verify that the Leap system is working (e.g. with Leap's visualizer software).

### Running splash

The Leap shared library must be on your `LD_LIBRARY_PATH` (or `DYLD_LIBRARY_PATH` if you're using a Mac) before you start splash.  e.g.:

  ```bash
    # With obi: LD_LIBRARY_PATH is set for you in the project.yaml file used by obi
    $ obi go
  ```

or

  ```bash
    # Without obi
    $ export DYLD_LIBRARY_PATH=$LEAPSDK_HOME/lib
    $ build/splash
  ```

### Verify it's working

  `$ peek gripes`

You should see the messages that splash is depositing into the **gripes** pool scrolling by quickly; when you hover a hand in view of the Leap, the data should resemble the output shown below in the **Output Format** section.


## Arguments

#### -o, --output

default:  gripes  
description: name of output pool

Any number of g-speak programs on the same machine can listen to (or "particpate in") that pool.

To connect on a remote machine. Be sure to run `pool-tcp-server` on the same machine running `splash`. Then on remote machines connect via:  `tcp://your-host-name-or-ip/gripes`.

#### -c, --config


default: none

Optionally, `splash` takes the fully qualified path to a simple YAML settings file that describes the physical location and orientation of the Leap sensor.  The location/orientation should make sense within the context of the screen protein you use with your g-speak application to define your space.

If no config is provided, splash will assume the leap is located at the origin of the room (0, 0, 0) with a default orientation of norm/over (0, 1, 0) / (1, 0, 0).

The config file format is as follows:

  ```yaml
    %YAML 1.1
    %TAG ! tag:oblong.com,2009:slaw/
    --- !protein
    descrips: # ignored
    ingests:
      leap:
        cent: [0, 0, 0]   # required, center of the leap device in absolute coordinates
        norm: [0, 1, 0]   # required, normal vector from the face of the leap device
        over: [1, 0, 0]   # required, vector pointing lengthwise along the device
  ```
`norm` is usually 'up' along the Y axis, because the Leap device is usually lying on a flat surface, parallel to the floor.

`over` is usually to the user's right (so it's positive in the X axis).

For more information on how g-speak works with 3D space refer to this [Spatial Considerations tutorial](https://platform.oblong.com/learning/spatial-operating/spatial_considerations/).

## Output Format

To see the output from `splash` in a human-readable format, use the peek command:

```bash
$ peek gripes # or the name of the output pool you specified at the command line.
```

See below for an example protein. There's a lot to unpack in these proteins but fortunately for you, g-speak apps will do that for you. The only lines that are likely of interest for you are the `gripe` values for the `LEFTISH` and `RIGHTISH` hands. These values represent the gesture splash (and the Leap sensor) believe your hands to be making at that time. Check out [this ancient document](https://platform.oblong.com/wp-content/uploads/2018/01/g-speak-gripes.pdf) for an overview of the gripe symbols and how to decipher them. Note: a `_____:__` value indicates that a gesture is currently not found. If you are getting valid input, you should see gripes like `^^^^>:-x` (which would indicate a fist with your thumb on top). Below is an example protein showing the left hand pointing with one finger, thumb curled in, and palm facing the floor while the right hand is pointing with one finger, thumb out, and palm facing left.

```yaml
descrips:
- gripeframe
ingests:
  origins:
  - name
  - leap-reader
  - clock
  - 162220055363
  time: 162220055363
  hands:
  - gripe: ^^^|>:vx
    type: LEFTISH
    loc: !vector [-92.518272399902344, 194.9117431640625, 22.707260131835938]
    aim: !vector [-0.0094077370313990112, -0.018713417875257328, -0.99978062717546901]
    back:
      loc: !vector [-92.518272399902344, 194.9117431640625, 22.707260131835938]
      norm: !vector [0.12501287733093325, 0.92654373283226743, 0.35480768262636142]
      over: !vector [0.98605561853476631, -0.15562093010566733, 0.058961370990380658]
      occluded: true
    fingers:
    - type: PINKY
      loc: !vector [-109.41815948486328, 152.60935974121094, 12.658197402954102]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: RING
      loc: !vector [-99.647392272949219, 139.78535461425781, 14.543222427368164]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: MIDDLE
      loc: !vector [-88.495918273925781, 135.19866943359375, 11.340911865234375]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: INDEX
      loc: !vector [-68.601226806640625, 204.47418212890625, -62.218631744384766]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: THUMB
      loc: !vector [-85.304008483886719, 165.55001831054688, -2.9797260761260986]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
  - gripe: ^^^|-:-x
    type: RIGHTISH
    loc: !vector [95.140037536621094, 188.48432922363281, 13.311689376831055]
    aim: !vector [-0.3574539035920351, -0.1326181557520599, -0.92446694455331302]
    back:
      loc: !vector [95.140037536621094, 188.48432922363281, 13.311689376831055]
      norm: !vector [0.99667198956339709, 0.042713508648656882, -0.06942983075494992]
      over: !vector [-0.045617955036583717, 0.99812544703445361, -0.040799438238193833]
      occluded: true
    fingers:
    - type: PINKY
      loc: !vector [67.867095947265625, 165.38813781738281, 6.5487613677978516]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: RING
      loc: !vector [62.203315734863281, 176.51449584960938, 5.4022808074951172]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: MIDDLE
      loc: !vector [54.803092956542969, 192.96875, 0.54686969518661499]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: INDEX
      loc: !vector [75.131324768066406, 200.17893981933594, -72.213287353515625]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
    - type: THUMB
      loc: !vector [85.564544677734375, 278.57528686523438, 19.203586578369141]
      norm: !vector [0.0, 0.0, 0.0]
      over: !vector [0.0, 0.0, 0.0]
      occluded: false
...
```
