/******************************************************************************
 *   Copyright (C) 2007 by Anton Maksimenko                                   *
 *   antonmx@post.kek.jp                                                      *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation; either version 2 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/


///
/// @file
/// @author antonmx <antonmx@gmail.com>
/// @date   Wed Jul  7 13:18:49 2010
///
/// @brief %All-in-one ct reconstruction of the inline phase contrast.
///
///

#include <vector>
#include "../common/common.h"
#include "../common/experiment.h"
#include "../common/ipc.h"
#include "../common/poptmx.h"
#include "../common/kernel.h"


using namespace std;


/// \CLARGS
struct clargs {
  Path command;               ///< Command name as it was invoked.
  IPCprocess::Component contrast; ///< Component to be extracted.
  Path z0_list;				///< File with the list of contrasts in the contact print plane.
  Path zD_list;				///< File with the list of contrasts at the distance.
  float dd;                     ///< Pixel size.
  float d2b;                  ///< \f$\d2b\f$ parameter of the MBA.
  float lambda;                 ///< Wavelength.
  float dist;                   ///< Object-to-detector distance.
  float dgamma;                 ///< \f$\gamma\f$ parameter of the BAC method
  float arc;
  string slicedesc;				///< String describing the slices to be CT'ed.
  Path outmask;				///< The mask for the output file names.
  Filter filter_type;           ///< Type of the filtering function.
  Dcenter center;               ///< Rotation center.
  unsigned nof_threads;         ///< Number of threads in the reconstruction.
  bool beverbose;				///< Be verbose flag
  bool SaveInt;					///< Save image as 16-bit integer.

  /// \CLARGSF
  clargs(int argc, char *argv[]);
};


clargs::
clargs(int argc, char *argv[]) :
  contrast(IPCprocess::PHS),
  z0_list(""),
  dd(1.0),
  d2b(0.0),
  lambda(1.0),
  dgamma(1.0),
  arc(180),
  outmask("sino-<input list>-@.tif"),
  SaveInt(false),
  nof_threads(0),
  beverbose(false)
{

  poptmx::OptionTable table
  ("Forms IPC-based sinogram(s).",
   "Performs following procedures:\n"
   "1) reads text file(s) describing the array(s) of input files (foreground-background"
   " pairs) in contact print plane and at a distance from the object.\n"
   "2) removes the backgrounds in accordance with these files.\n"
   "3) performs the IPC extraction of the requested contrast component.\n"
   "4) constructs the sinograms of the slices requested in the slice string.");

  table
  .add(poptmx::NOTE, "ARGUMENTS:")
  .add(poptmx::ARGUMENT, &zD_list, "far-intensity", "List of input images taken at the distance.", "")
  .add(poptmx::ARGUMENT, &z0_list, "0-intensity", "List of input images taken in the contact print plane"
       " (clean absorption contrast).", "", "NONE")
  .add(poptmx::NOTE, "", "Arguments \"" + table.desc(&zD_list) + "\" and \"" + table.desc(&z0_list)
       + "\". " + AqSeries::Desc)

  .add(poptmx::NOTE, "OPTIONS:")
  .add(poptmx::OPTION, &outmask, 'o', "out",
       "Output result mask.", MaskDesc, outmask)
  .add(poptmx::OPTION, &contrast, 'C', "contrast",
       "Type of the contrast component.",
       "The component of the contrast to extract"
       " and then reconstruct. " + IPCprocess::componentDesc, toString(contrast))
	.add(poptmx::OPTION, &dist, 'z', "distance", "Object-to-detector distance (mm)",
         "More correctly the distance from the contact print plane and the detector plane where the image"
         " given by the argument " + table.desc(&zD_list) + " was taken. " + NeedForQuant)
	.add(poptmx::OPTION, &dd, 'r', "resolution", "Pixel size of the detector (micron)",
         NeedForQuant, toString(dd))
	.add(poptmx::OPTION, &d2b, 'd', "d2b", "delta/beta ratio.", "", toString(d2b))
  .add(poptmx::OPTION, &lambda, 'w', "wavelength", "Wavelength of the X-Ray (Angstrom)",
         "Only needed together with " + table.desc(&d2b) + ".", toString(lambda))
  .add(poptmx::OPTION, &dgamma, 'g', "gamma", "Gamma coefficient of the BAC.",
       "Must be a value around 1.0 (theoretical).", toString(dgamma))
  .add(poptmx::OPTION, &slicedesc, 's', "slice",
       "Slices to be processed.", SliceOptionDesc, "<all>")
  .add(poptmx::OPTION,   &center, 'c', "center",
       "Variable rotation center.", DcenterOptionDesc, toString(0.0))
  .add(poptmx::OPTION, &arc, 'a', "arcan",
       "CT scan range (if > 1 deg), or step size (if < 1deg).",
       "If value is greater than 1deg then it represents the arc of the CT scan."
       " Otherwise the value is the step size.",
       toString(arc))
  .add(poptmx::OPTION,   &filter_type, 'f', "filter",
       "Filtering window used in the CT.", FilterOptionDesc, filter_type.name())
  .add(poptmx::OPTION,   &nof_threads, 't', "threads",
       "Number of threads used in calculations.",
       "If the option is not used the optimal number is"
       " calculated automatically.", "<auto>")
  .add(poptmx::OPTION, &SaveInt,'i', "int",
       "Output image(s) as integer.", IntOptionDesc)
  .add_standard_options(&beverbose)
  .add(poptmx::MAN, "SEE ALSO:", SeeAlsoList);

  if ( ! table.parse(argc,argv) )
    exit(0);
  if ( ! table.count() ) {
    table.usage();
    exit(0);
  }

  command = table.name();

  // <minus list> and <plus list> : required arguments.
  if ( ! table.count(&zD_list) )
    exit_on_error(command, "Missing required argument: "+table.desc(&zD_list)+".");

  if ( ! table.count(&outmask) )
    outmask = upgrade(outmask, "reconstructed-"+zD_list) + "-@.tif";
  if ( string(outmask).find('@') == string::npos )
    outmask = outmask.dtitle() + "-@" + outmask.extension();

  if ( ! table.count(&contrast) )
    exit_on_error(command, "Missing required argument: "+table.desc(&contrast)+".");

  if ( ! table.count(&dist) )
    exit_on_error(command, "Missing required option: "+table.desc(&dist)+".");
  if (dist <= 0.0)
    exit_on_error(command, "Zero or negative distance (given by "+table.desc(&dist)+").");
  dist /= 1.0E3; // convert mm -> m

  if (dd <= 0.0)
    exit_on_error(command, "Zero or negative pixel size (given by "+table.desc(&dd)+").");
  dd /= 1.0E6; // convert micron -> m

  if ( abs(dgamma)>1.0 ) // should set even smaller limit
    exit_on_error(command, "Absolute value of gamma (given by "+table.desc(&dgamma)+")"
                  " is greater than 1.0.");

  if (lambda <= 0.0)
    exit_on_error(command, "Zero or negative wavelength (given by "+table.desc(&lambda)+").");
  if ( table.count(&lambda) && ! table.count(&d2b) )
    warn(command, "The wavelength (given by "+table.desc(&lambda)+") has influence only together"
         " with the alpha parameter (given by "+table.desc(&d2b)+").");
  if ( ! table.count(&lambda) && table.count(&d2b) )
    warn(command, "The wavelength (given by "+table.desc(&lambda)+") needed together with"
         " the alpha parameter (given by "+table.desc(&d2b)+") for the correct results.");
  lambda /= 1.0E10; // convert A -> m

  if (d2b < 0.0)
    exit_on_error(command, "Negative alpha parameter (given by "+table.desc(&d2b)+").");
  if (arc <= 0.0)
    exit_on_error(command, "CT arc (given by "+table.desc(&arc)+") must be strictly positive.");



}



/// \MAIN{sinr}
int main(int argc, char *argv[]) {

  const clargs args(argc, argv);
  const AqSeries listD(args.zD_list);
  const AqSeries list0(args.z0_list);
  IPCprocess proc(listD.shape(), M_PI * args.d2b * args.dist * args.lambda / ( args.dd * args.dd ) );
  IPCexp expr(listD, list0, proc, args.contrast);
  expr.gamma(args.dgamma);

  const int
    thetas=expr.thetas(),
    pixels=expr.pixels(),
    slices=expr.slices();
  const string sliceformat = mask2format(args.outmask, slices);
  const vector<int> sliceV = slice_str2vec(args.slicedesc, slices);
  const SinoS sins(expr, sliceV, args.beverbose);
  const float arc = args.arc > 1.0 ? args.arc : args.arc * thetas;

  CTrec rec(expr.shape(), expr.contrast(), arc, args.filter_type);

  Map sinogram(thetas, pixels);
  Map result(thetas, pixels);
  ProgressBar bar(args.beverbose, "sinogram formation", sliceV.size());

  for (unsigned slice=0 ; slice < sliceV.size() ; slice++ ) {
    sins.sino(slice, sinogram);
    const Map & res = rec.reconstruct(sinogram, args.center(sliceV[slice]+1), args.dd);
    SaveImage( toString(sliceformat, sliceV[slice]+1), res, args.SaveInt);
    bar.update();
  }

  exit(0);

}
