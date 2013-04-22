# Splash

A [Greenhouse](http://greenhouse.oblong.com/) worker that transforms
the Leap Motion's relative coordinates to real-world coordinates and
deposits each transformed frame into a local pool, 'leap'.

## Building

Make sure to set `LEAPSDK_HOME` to the directory where your Leap
SDK lives.  From there it should just be a matter of running `make`.

    $ LEAPSDK_HOME=/path/to/leapsdk make

### Leap SDK version recommendation

`splash` is meant to run as a background process, feeding proteins to other
foregrounded applications, perhaps running on multiple computers.  Version
0.7.6 of Leap's SDK is currently recommended, as it will deliver tracking
data to listeners regardless of whether or not they have system focus.


## Running

    $ ./splash

Make sure that the Leap shared library is on your `LD_LIBRARY_PATH`
(or `DYLD_LIBRARY_PATH` if you're using a Mac) before you start
splash.


### Arguments

Optionally, `splash` takes the fully qualified path to a protein file
that describes its location.  Its format is as follows:

    descrips: # ignored
    ingests:
      leap:
        cent: [0, 0, 0] # v3float64, required, real-space location
        norm: [0, 1, 0] # v3float64, required, real-space normal vector
        over: [1, 0, 0] # v3float64, required, real-space over vector
        provenance: "leap-whatever" # Str, optional, defaults to "leap-$HOSTNAME"

If a protein file is not supplied on the command line, `splash` will
try to find the same ingest key in the `/etc/oblong/screen.protein` file.

If the appropriate ingest keys are not found in the `screen.protein` file,
`splash` will assume that your Leap is 500mm in front and 200mm below the
center of the "main" screen described in your `screen.protein` file.

If a `screen.protein` file can not be found, or if it can and it does not
contain a screen called "main", `splash` will warn you of as much and will
pass the leap's data through in its native, relative space.

For more information on Greenhouse's spatial understanding refer to this
[Spatial Considerations tutorial](http://greenhouse.oblong.com/learning/spatial.html).

### Environment

By default, `splash` deposits Leap proteins to a local pool called "leap". You
can specify a different pool to deposit the Leap's information in to via the
`LEAP_POOL` environment variable.  Note that this pool must exist before you run
`splash.`

    $ p-create my_new_leap_pool
    $ LEAP_POOL=my_new_leap_pool ./splash

## Output Format

`splash` produces proteins of the following format:

    descrips:
    - greenhouse
    - leap
    - 0.7.6 # The version of the Leap Motion API
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

