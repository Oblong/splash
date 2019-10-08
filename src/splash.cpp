
/* (c)  oblong industries */

// # splash
//
// Help your Leap Motion sensor dive in to pools

#include <Leap.h>

#include <libLoam/c++/Str.h>
#include <libPlasma/c++/Pool.h>
#include <libBasement/UrDrome.h>
#include <libStaging/libBasement/Hasselhoff.h>
#include <libStaging/libPlasma/c++/PlasmaHelpers.h>

#include <list>
#include <boost/thread/mutex.hpp>

using namespace oblong::basement;
using namespace oblong::plasma;
using namespace oblong::loam;

#define MIN_CONFIDENCE 0.4

/// FrameWriter: A handy class to transform leap types into plasma types
class FrameWriter
{
 protected:
  Leap::Vector origin, normal, over;
  Leap::Matrix transform;
  //  Str provenance;

  struct phalange
  {
    Str type;
    Vect loc, norm, over;
    bool occluded;
    phalange (const Str &t)
    {
      type = t;
      occluded = true;
    }
    Slaw toSlaw ()
    {
      return Slaw::Map ("type", type, "loc", loc, "norm", norm, "over", over,
                        "occluded", occluded);
    }
  };

  /// Transform g-speak Vect into Leap Vector
  const Leap::Vector LV (Vect const &v) const
  {
    return Leap::Vector (v.x, v.y, v.z);
  }

  const Vect GV (Leap::Vector const &v) const { return Vect (v.x, v.y, v.z); }

  /// Takes a relative direction from Leap and returns it's absolute direction
  Leap::Vector const Direction (Leap::Vector const &v) const
  {
    return transform.transformDirection (v);
  }
  /// Takes a relative Location from Leap and returns it's absolute location
  Leap::Vector const Point (Leap::Vector const &v) const
  {
    return transform.transformPoint (v);
  }

  Slaw ProcessFingers (Leap::FingerList const &fl, const Vect &pNorm,
                       const Vect &pUp, Vect *aim, Str &gripe) const
  {
    char gstring[] = "_____";
    phalange phalanges[5] = {phalange ("THUMB"), phalange ("INDEX"),
                             phalange ("MIDDLE"), phalange ("RING"),
                             phalange ("PINKY")};
    // for (int i = 0)
    // Str types [5] = { "THUMB", "INDEX", "MIDDLE", "RING", "PINKY" };
    for (int i = fl.count () - 1; i >= 0; i--)
      {
        Leap::Finger f = fl[i];
        int t = f.type ();
        phalange p = phalanges[t];
        p.loc = GV (Point (f.stabilizedTipPosition ()));
        p.occluded = false;
        if (f.isExtended ())
          {
            if (t == 0)
              {
                Vect dir = GV (Direction (f.direction ()));
                gstring[4 - t] = dir.AngleWith (pUp) < (0.3 * M_PI) ? '-' : '|';
              }
            else
              {
                gstring[4 - t] = '|';
                if (t == 1 && aim)
                  {
                    aim->Set (GV (Direction (f.direction ())));
                    aim->NormSelf ();
                  }
              }
          }
        else
          {
            if (t == 0)
              gstring[4 - t] = '>';
            else
              {
                Vect dir = GV (Direction (f.direction ()));
                gstring[4 - t] = dir.AngleWith (pNorm) < M_PI / 4.0 ? 'x' : '^';
              }
          }
        phalanges[t] = p;
      }
    gripe = Str ().Sprintf ("%s", gstring);
    return Slaw::List (phalanges[4].toSlaw (), phalanges[3].toSlaw (),
                       phalanges[2].toSlaw (), phalanges[1].toSlaw (),
                       phalanges[0].toSlaw ());
  }

  // Here we possibly incorrectly assume that the room's norm and over
  // are (0, 0, 1) and (1, 0, 0) respectively and that the person is
  // aligned with the leap and the room
  Str directionalGripe (Vect dir, bool leftish) const
  {
    float64 min_angle = 4.0 * M_PI;
    int32 min_dir = -1;

    Vect dirs[6] = {Vect (1, 0, 0),  Vect (-1, 0, 0), Vect (0, 1, 0),
                    Vect (0, -1, 0), Vect (0, 0, 1),  Vect (0, 0, -1)};

    for (int i = 0; i < 6; i++)
      {
        float64 angle = dir.AngleWith (dirs[i]);
        if (angle < min_angle)
          {
            min_dir = i;
            min_angle = angle;
          }
      }

    switch (min_dir)
      {
        case 0:
          return leftish ? "-" : "+";  // lateral/medial
        case 1:
          return leftish ? "+" : "-";  // lateral/medial
        case 2:
          return "^";  // cranial
        case 3:
          return "v";  // caudal
        case 4:
          return ".";  // posterior
        case 5:
          return "x";  // anterior
        default:
          return "_";  // unknown
      };

    return "_";
  }

  Slaw MissingHand (const Str &type) const
  {
    Vect v;
    return Slaw::Map ("gripe", "_____:__", "type", type, "loc", v, "aim", v,
                      "back", Slaw::Map ("loc", v, "norm", v, "over", v,
                                         "occluded", true),
                      "fingers", Slaw::List (phalange ("PINKY").toSlaw (),
                                             phalange ("RING").toSlaw (),
                                             phalange ("MIDDLE").toSlaw (),
                                             phalange ("INDEX").toSlaw (),
                                             phalange ("THUMB").toSlaw ()));
  }

  Slaw ToSlaw (Leap::Hand const &h) const
  {
    Vect loc = GV (Point (h.stabilizedPalmPosition ()));
    Vect aim = GV (Direction (h.direction ())).Norm ();
    Vect pnorm = GV (Direction (h.palmNormal ())).Norm ();
    Vect norm = -pnorm;
    Vect up = norm.Cross (aim);
    Str type = h.isLeft () ? "LEFTISH" : "RIGHTISH";
    bool occ = h.confidence () >= MIN_CONFIDENCE;
    Str gripe = "_____:__";
    Slaw fingers = ProcessFingers (h.fingers (), pnorm, up, &aim, gripe);
    gripe = gripe + ":" + directionalGripe (pnorm, h.isLeft ())
            + directionalGripe (aim, h.isLeft ());
    return Slaw::Map ("gripe", gripe, "type", type, "loc", loc, "aim", aim,
                      "back", Slaw::Map ("loc", loc, "norm", norm, "over", up,
                                         "occluded", occ),
                      "fingers", fingers);
  }

  /// Creates a slaw that fully describes a Hand found by the Leap
  Slaw ToSlaw (Leap::HandList const &hl) const
  {
    if (hl.count () == 1)
      {
        Leap::Hand h = hl[0];
        Str missing = h.isLeft () ? "RIGHTISH" : "LEFTISH";
        return Slaw::List (ToSlaw (h), MissingHand (missing));
      }
    else if (hl.count () == 0)
      {
        return Slaw::List (MissingHand ("RIGHTISH"), MissingHand ("LEFTISH"));
      }

    Slaw hands = Slaw::List ();
    for (int i = 0; i < hl.count (); i++)
      hands = hands.ListAppend (ToSlaw (hl[i]));
    return hands;
  }

 public:
  FrameWriter () : transform (Leap::Matrix::identity ()) {}

  /// Sets up the location and orientation of the leap in order to properly
  /// transform relatively located leap events into absolutely located ones.
  void SetLocAndOrientation (Vect const &orig, Vect const &nrm, Vect const &ov)
  {
    origin = LV (orig);
    normal = LV (nrm);
    over = LV (ov);
    transform = Leap::Matrix (over, normal, over.cross (normal), origin);
  }

  /// Takes an event/frame generated from a leap and returns a Slaw that fully
  /// describes that event
  Slaw ToSlaw (Leap::Frame const &frame) const
  {
    return ToSlaw (frame.hands ());
  }
};

/// Callback handler for Leap events
template <typename Callback>
class SplashListener : public Leap::Listener
{
 protected:
  Callback &callback;

 public:
  SplashListener (Callback &c) : callback (c) {}
  virtual void onFrame (const Leap::Controller &c)
  {
    callback.DepositGripes (c.frame ());
  }
};

// Surely there's a way to get this from Leap's API...?
static const Str LEAP_VERSION = "0.7.9";
static const float64 DEFAULT_Z_DISTANCE = 500.0;
static const float64 DEFAULT_Y_DISTANCE = 200.0;

/// The main class type for Splash. It sets up the leap controller, the
/// callback handler for the controller and handles each frame using
/// the FrameWriter class to transform a leap frame into a g-speak protein
/// and deposits the protein into the designated output pool.
class Splash : public KneeObject
{
 private:
  Str const pool;
  SplashListener<Splash> listener;
  Leap::Controller controller;
  boost::mutex mutex;
  FrameWriter writer;
  std::list<Protein> to_deposit;
  bool stop_depositing;
  Str orig;

 public:
  Splash (Str const &output_pool)
      : KneeObject (),
        pool (output_pool),
        listener (*this),
        controller (listener),
        stop_depositing (false)
  {  // Initialize Hasselhoff and the leap pool for pool participation
    oblong::staging::Hasselhoff::TheMainMan ()->PoolParticipate (pool, pool,
                                                                 NULL);
    controller.setPolicyFlags (Leap::Controller::POLICY_BACKGROUND_FRAMES);
    orig = "leap-reader-v" + LEAP_VERSION;
  }

  virtual ~Splash ()
  {  // Make sure we don't try to deposit any more proteins after
    // the application shuts down.
    {
      boost::mutex::scoped_lock lck (mutex);
      stop_depositing = true;
    }
    if (controller.isConnected ())
      controller.removeListener (listener);
  }

  /// The handler function for each Leap frame, it takes a leap frame,
  /// transforms it to a protein and deposits it to the leap pool
  void DepositGripes (Leap::Frame const &f)
  {
    int64 tim = controller.now ();  //FatherTime::AbsoluteTime ();
    Protein p (Slaw::List ("gripeframe"),
               Slaw::Map ("origins", Slaw::List ("name", orig, "clock", tim),
                          "time", tim, "hands", writer.ToSlaw (f)));

    // The leap runs in its own thread.  Make sure that we don't try
    // to deposit two proteins at the same time.
    boost::mutex::scoped_lock lck (mutex);
    if (!stop_depositing)
      to_deposit.push_back (p);
  }

  virtual ObRetort Travail (Atmosphere *atm) override
  {
    std::list<Protein> copy;
    {
      boost::mutex::scoped_lock lck (mutex);
      std::swap (copy, to_deposit);
    }
    for (Protein const &p : copy)
      oblong::staging::Hasselhoff::TheMainMan ()->PoolDeposit (pool, p);

    return OB_OK;
  }

  /// Reads in a configuration protein to set up the spatial layout of
  /// the leap
  bool Configure (Protein const &p)
  {
    Slaw ing = p.Ingests ();
    Vect origin, normal, over;
    Str prov;
    Slaw leap = ing.MapFind ("leap");
    // if (leap . MapFind ("provenance") . Into (prov))
    //   writer . SetProvenance (prov);

    if (leap.MapFind ("cent").Into (origin)
        && leap.MapFind ("norm").Into (normal)
        && leap.MapFind ("over").Into (over))
      {
        writer.SetLocAndOrientation (origin, normal, over);
        return true;
      }

    // If there's no explicit Leap information, infer it from
    // the screen protein, using the ``main'' screen.  We'll
    // assume that the leap is about 50cm back and 20 cm down
    // from the center of the screen and is pointing up.
    Slaw screen = ing.MapFind ("screens").MapFind ("main");
    if (screen.MapFind ("cent").Into (origin)
        && screen.MapFind ("over").Into (over))
      {
        normal = Vect (0, 1, 0);
        origin -= DEFAULT_Y_DISTANCE * normal.Norm ()
                  + DEFAULT_Z_DISTANCE * normal.Cross (over).Norm ();
        writer.SetLocAndOrientation (origin, normal, over);
        return true;
      }
    return false;
  }
};

static const char *DEFAULT_POOL = "leap";

int main (int argc, char **argv)
{
  UrDrome *ud = new UrDrome ("splash", argc, argv);

  const char *leap_pool = getenv ("LEAP_POOL");
  Splash *splash = new Splash (NULL == leap_pool ? DEFAULT_POOL : leap_pool);
  ud->AppendChild (splash);

  bool loaded = false;
  if (1 < argc)
    {
      Protein p = oblong::staging::LoadProtein (argv[1]);
      OB_LOG_WARNING ("Configuring leap from: %s", argv[1]);
      if (!p.IsNull ())
        loaded = splash->Configure (p);
    }
  if (!loaded)
    {
      Str screen = "/etc/oblong/screen.protein";
      Protein p = oblong::staging::LoadProtein (screen);
      OB_LOG_WARNING ("/etc/oblong/screen.protein");
      if (!p.IsNull ())
        loaded = splash->Configure (p);
    }
  if (!loaded)
    OB_LOG_WARNING ("Could not find or infer configuration information for "
                    "your LeapMotion");

  ud->SetRespirePeriod (1 / 100.0);
  ud->Respire ();
  ud->Delete ();
}
