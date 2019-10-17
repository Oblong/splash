
/* (c)  oblong industries */

const char *Usage(R"(
Overview:
  Splash is a g-speak program that transforms Leap Motion's relative
  coordinates to absolute coordinates and converts the hand positions
  into g-speak's own gestural language, known as 'gripes'. Splash
  streams its data output as a series of messages into a pool.

To exit: 'Ctrl + c' or 'obi stop' in the terminal
)");

#include <Leap.h>

#include <libLoam/c++/ArgParse.h>
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
    if (h.isLeft ())
      up = -up;
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
    orig = "leap-reader";
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
    // If no explicit Leap configuration is provided, assume
    // the leap is located at the room origin with a typical
    // norm/over ([0, 1, 0] / [1, 0, 0])
    Vect origin;
    Vect normal (0, 1, 0);
    Vect over (1, 0, 0);
    if (!p.IsNull ())
      {
        Slaw leap = p.Ingests ().MapFind ("leap");
        if (!leap.MapFind ("cent").Into (origin)
            || !leap.MapFind ("norm").Into (normal)
            || !leap.MapFind ("over").Into (over))
          return false;
      }
    writer.SetLocAndOrientation (origin, normal, over);

    OB_LOG_INFO ("Running splash:\n"
                 "  Output Pool: %s\n"
                 "  Leap Configuration:\n"
                 "    Loc:   %s\n"
                 "    Norm:  %s\n"
                 "    Over:  %s", pool.utf8 (), origin.AsStr ().utf8 (),
                 normal.AsStr ().utf8 (), over.AsStr ().utf8 ());
    return true;
  }
};

int main (int argc, char **argv)
{
  Str leap_config, leap_pool = "gripes";
  ArgParse ap (argc, argv);
  ap.ArgString ("output", "\aname of output pool", &leap_pool, true);
  ap.ArgString ("config", "\apath to screen or leap configuration file",
                &leap_config);
  ap.Alias ("output", "o");
  ap.Alias ("config", "c");
  ap.UsageHeader(Usage);
  ap.EasyFinish (0, 0);

  // Create Drome
  UrDrome *ud = new UrDrome("splash", ap.Leftovers());

  Splash *splash = new Splash (leap_pool);
  ud->AppendChild (splash);

  Protein p = Protein::Null ();
  if (!leap_config.IsEmpty ())
    p = oblong::staging::LoadProtein (leap_config);
  if (!splash->Configure (p))
    OB_FATAL_ERROR("Could not parse leap configuration file: %s",
                   leap_config.utf8 ());

  ud->SetRespirePeriod (1 / 100.0);
  ud->Respire ();
  ud->Delete ();
}
