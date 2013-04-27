
/* (c)  oblong industries */

// # splash
//
// Help your Leap Motion sensor dive in to pools

#include <Leap.h>
#define WORKER
#include <Greenhouse.h>
#include <list>
#include <boost/thread/mutex.hpp>

/// FrameWriter: A handy class to transform leap types into plasma types
class FrameWriter {
protected:
  Leap::Vector origin, normal, over;
  Leap::Matrix transform;
  Str provenance;

  /// Transform g-speak Vect into Leap Vector
  const Leap::Vector VV (Vect const& v) const
  { return Leap::Vector (v.x, v.y, v.z); }

  /// Takes a relative direction from Leap and returns it's absolute direction
  Leap::Vector const Direction (Leap::Vector const& v) const
  { return transform . transformDirection (v); }
  /// Takes a relative Location from Leap and returns it's absolute location
  Leap::Vector const Point (Leap::Vector const& v) const
  { return transform . transformPoint (v); }

  /// Transforms a Leap Vector in to a g-speak Vect and wraps it in a Slaw
  Slaw ToSlaw (Leap::Vector const &v) const
  { return Slaw (Vect (v.x, v.y, v.z)); }

  /// Transforms a list into a Slaw list
  template <typename Lst>
  Slaw ToSlaw (Lst const &ll) const {
    Slaw out = Slaw::List ();
    for (auto &x : ll)
      out = out . ListAppend (ToSlaw (x));
    return out;
  }

  /// Creates a slaw that fully describes a Leap Pointing gesture
  Slaw ToSlaw (Leap::Pointable const& p) const
  { Leap::Vector
      phys_origin = Point (p . tipPosition ()),
      phys_through = phys_origin + p . length () * Direction (p . direction ());
    return Slaw::Map ("id", p . id (),
                      "dir", ToSlaw (Direction (p . direction ())),
                      "hand", p . hand () . id (),
                      "isfngr", p . isFinger (),
                      "istool", p . isTool (),
                      "length", p . length ())
      . MapMerge (Slaw::Map ("t-pos", ToSlaw (phys_origin),
                             "t-vel", ToSlaw (p . tipVelocity ()),
                             "width", p . width (),
                             "orig", ToSlaw (phys_origin),
                             "thru", ToSlaw (phys_through)));
  }

  /// Creates a slaw that fully describes a Hand found by the Leap
  Slaw ToSlaw (Leap::Hand const& h) const
  { Leap::Vector
      phys_origin  = Point (h . palmPosition ()),
      phys_through = phys_origin + Direction (h . direction ());
    return Slaw::Map ("id", h . id (),
                      "dir", ToSlaw (Direction (h . direction ())),
                      "plmnrm", ToSlaw (Direction (h . palmNormal ())),
                      "plmpos", ToSlaw (phys_origin),
                      "plmvel", ToSlaw (h . palmVelocity ()),
                      "center", ToSlaw (Direction (h . sphereCenter ())),
                      "radius", h . sphereRadius (),
                      "orig", ToSlaw (phys_origin),
                      "thru", ToSlaw (phys_through));
  }

  /// Returns a Leap::Gesture::State as a slaw wrapped string
  Slaw ToSlaw (Leap::Gesture::State const& s) const
  { switch (s) {
    case Leap::Gesture::STATE_START: return Slaw ("start");
    case Leap::Gesture::STATE_UPDATE: return Slaw ("update");
    case Leap::Gesture::STATE_STOP: return Slaw ("stop");
    default: return Slaw ("invalid");
    }
  }


  /// Transforms a list into a Slaw list of just id's
  template <typename Lst>
  Slaw IDs (Lst const& lst) const
  { Slaw out = Slaw::List ();
    for (auto &x : lst)
      out = out . ListAppend (x . id ());
    return out;
  }

  /// Creates a slaw that fully describes a Leap Gesture
  Slaw ToSlaw (Leap::Gesture const& g) const
  { Slaw out = Slaw::Map ("id", g . id (),
                          "dur", g . duration (),
                          "dursec", g . durationSeconds (),
                          "hands", IDs (g . hands ()),
                          "pntrs", IDs (g . pointables ()),
                          "state", ToSlaw (g . state ()));
    switch (g . type ()) {
    case Leap::Gesture::TYPE_SWIPE:
      return ToSlaw ((Leap::SwipeGesture const&) g, out);
    case Leap::Gesture::TYPE_CIRCLE:
      return ToSlaw ((Leap::CircleGesture const&) g, out);
    case Leap::Gesture::TYPE_SCREEN_TAP:
      return ToSlaw ((Leap::ScreenTapGesture const&) g, out);
    case Leap::Gesture::TYPE_KEY_TAP:
      return ToSlaw ((Leap::KeyTapGesture const&) g, out);
    default:
      out = out . MapPut ("type", "invalid");
      return out;
    }
  }

  /// Creates a slaw that fully describes a Leap "circle" gesture
  Slaw ToSlaw (Leap::CircleGesture const& c, Slaw out) const
  { return out . MapMerge
      (Slaw::Map ("type", "circle",
                  "point", c . pointable () . id (),
                  "cent", ToSlaw (Point (c . center ())),
                  "norm", ToSlaw (Direction (c . normal ())),
                  "prog", c . progress (),
                  "radius", c . radius ()));
  }

  /// Creates a slaw that fully describes a Leap "swipe" gesture
  Slaw ToSlaw (Leap::SwipeGesture const& s, Slaw out) const
  { return out . MapMerge
      (Slaw::Map ("type", "swipe",
                  "dir", ToSlaw (Direction (s . direction ())),
                  "point", s . pointable () . id (),
                  "pos", ToSlaw (Point (s . position ())),
                  "speed", s . speed (),
                  "start", ToSlaw (Point (s . startPosition ()))));
  }

  /// Creates a slaw that fully describes a Leap "screen tap" gesture
  Slaw ToSlaw (Leap::ScreenTapGesture const& s, Slaw out) const
  { return out . MapMerge
      (Slaw::Map ("type", "s-tap",
                  "dir", ToSlaw (Direction (s . direction ())),
                  "point", s . pointable () . id (),
                  "pos", ToSlaw (Point (s . position ())),
                  "prog", s . progress ()));
  }

  /// Creates a slaw that fully describes a Leap "key tap" gesture
  Slaw ToSlaw (Leap::KeyTapGesture const& s, Slaw out) const
  { return out . MapMerge
      (Slaw::Map ("type", "k-tap",
                  "dir", ToSlaw (Direction (s . direction ())),
                  "point", s . pointable () . id (),
                  "pos", ToSlaw (Point (s . position ())),
                  "prog", s . progress ()));
  }

  /// returns the hostname of the machine running splash
  static const Str Hostname ()
  { const size_t MAX_HOSTNAME = 64;
    char hostname[MAX_HOSTNAME];
    if (-1 != gethostname (hostname, MAX_HOSTNAME))
      return Str (hostname);
    else
      return Str ();
  }

  /// creates a provenance that uniquely identifies the leap connected to the
  /// machine running splash using the hostname
  static const Str DefaultProvenance ()
  { return "leap-" + Hostname (); }

public:
  FrameWriter ()  :  transform (Leap::Matrix::identity ()),
                     provenance (DefaultProvenance ()) {}

  void SetProvenance (Str const &p) { provenance = p; }

  /// Sets up the location and orientation of the leap in order to properly
  /// transform relatively located leap events into absolutely located ones.
  void SetLocAndOrientation (Vect const& orig, Vect const& nrm, Vect const& ov)
  { origin = VV (orig);
    normal = VV(nrm);
    over = VV(ov);
    transform = Leap::Matrix (over, normal, over . cross (normal), origin);
  }

  /// Takes an event/frame generated from a leap and returns a Slaw that fully
  /// describes that event
  Slaw ToSlaw (Leap::Frame const& frame) const
  { return Slaw::Map ("ts", frame . timestamp (),
                      "id", frame . id (),
                      "hands", ToSlaw (frame . hands ()),
                      "pntrs", ToSlaw (frame . pointables ()),
                      "gests", ToSlaw (frame . gestures ()));
  }

  /// Returns a slaw the provides the leap's location, orientation and name
  Slaw DeviceDescription () const
  { return Slaw::Map ("orig", ToSlaw (origin),
                      "norm", ToSlaw (normal),
                      "over", ToSlaw (over),
                      "prov", provenance);
  }
};

/// Callback handler for Leap events
template <typename Callback>
class SplashListener  :  public Leap::Listener {
protected:
  Callback &callback;
public:
  SplashListener (Callback &c)
    : callback (c) {}
  virtual void onFrame (const Leap::Controller &c)
  { callback . DepositFrame (c . frame ()); }
};

// Surely there's a way to get this from Leap's API...?
static const Str LEAP_VERSION = "0.7.6";
static const float64 DEFAULT_Z_DISTANCE = 500.0;
static const float64 DEFAULT_Y_DISTANCE = 200.0;

/// The main class type for Splash. It sets up the leap controller, the
/// callback handler for the controller and handles each frame using
/// the FrameWriter class to transform a leap frame into a g-speak protein
/// and deposits the protein into the designated output pool.
class Splash  :  public Thing {
private:
  Str const pool;
  SplashListener <Splash> listener;
  Leap::Controller controller;
  boost::mutex mutex;
  FrameWriter writer;
  std::list<Protein> to_deposit;
  bool stop_depositing;
public:
  Splash (Str const& output_pool)
    :  Thing (),
       pool (output_pool),
       listener (*this),
       controller (listener),
       stop_depositing (false)
  { ParticipateInPool (pool);
    controller . enableGesture (Leap::Gesture::TYPE_CIRCLE);
    controller . enableGesture (Leap::Gesture::TYPE_SWIPE);
    controller . enableGesture (Leap::Gesture::TYPE_SCREEN_TAP);
    controller . enableGesture (Leap::Gesture::TYPE_KEY_TAP);
    controller . setPolicyFlags (Leap::Controller::POLICY_BACKGROUND_FRAMES);
  }

  virtual ~Splash ()
  { // Make sure we don't try to deposit any more proteins after
    // the application shuts down.
    { boost::mutex::scoped_lock lck (mutex);
      stop_depositing = true;
    }
    if (controller . isConnected ())
      controller . removeListener (listener);
  }

  /// The handler function for each Leap frame, it takes a leap frame,
  /// transforms it to a protein and deposits it to the leap pool
  void DepositFrame (Leap::Frame const& f)
  { Protein p (Slaw::List ("greenhouse", "leap", LEAP_VERSION),
               Slaw::Map ("leap", writer . DeviceDescription (),
                          "frame", writer . ToSlaw (f)));

    // The leap runs in its own thread.  Make sure that we don't try
    // to deposit two proteins at the same time.
    boost::mutex::scoped_lock lck (mutex);
    if (! stop_depositing)
      to_deposit . push_back (p);
  }

  virtual void Travail ()
  { std::list<Protein> copy;
    { boost::mutex::scoped_lock lck (mutex);
      std::swap (copy, to_deposit);
    }
    for (Protein const& p : copy)
      Deposit (p, pool);
  }

  /// Reads in a configuration protein to set up the spatial layout of
  /// the leap
  bool Configure (Protein const& p)
  { Slaw ing = p . Ingests ();
    Vect origin, normal, over;
    Str prov;
    Slaw leap = ing . MapFind ("leap");
    if (leap . MapFind ("provenance") . Into (prov))
      writer . SetProvenance (prov);

    if (leap . MapFind ("cent") . Into (origin) &&
        leap . MapFind ("norm") . Into (normal) &&
        leap . MapFind ("over") . Into (over))
      { writer . SetLocAndOrientation (origin, normal, over);
        return true;
      }

    // If there's no explicit Leap information, infer it from
    // the screen protein, using the ``main'' screen.  We'll
    // assume that the leap is about 50cm back and 20 cm down
    // from the center of the screen and is pointing up.
    Slaw screen = ing . MapFind ("screens") . MapFind ("main");
    if (screen . MapFind ("cent") . Into (origin) &&
        screen . MapFind ("over") . Into (over))
      { normal = Vect (0, 1, 0);
        origin -= DEFAULT_Y_DISTANCE * normal . Norm ()
          + DEFAULT_Z_DISTANCE * normal . Cross (over) . Norm ();
        writer . SetLocAndOrientation (origin, normal, over);
        return true;
      }
    return false;
  }
};

static const char* DEFAULT_POOL = "leap";

void Setup ()
{ const char *leap_pool = getenv ("LEAP_POOL");
  Splash *splash = new Splash (NULL == leap_pool ?
                               DEFAULT_POOL : leap_pool);
  bool loaded = false;
  if (0 < args . Count ())
    { Protein p = LoadProtein (args [0]);
      if (! p . IsNull ())
        loaded = splash -> Configure (p);
    }
  if (! loaded)
    { Protein p = LoadProtein ("/etc/oblong/screen.protein");
      if (! p . IsNull ())
        loaded = splash -> Configure (p);
    }
  if (! loaded)
    WARN ("Could not find or infer configuration information for your LeapMotion");
}
