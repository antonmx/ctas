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
/// @author antonmx <antonmx@synchrotron.org.au>
/// @date   Thu Jul 17 12:51:00 2008
///
/// @brief Definitions of function declared in common.h
///
/// Here the functions declared in common.h are defined.
///

#ifdef _WIN32
#  define _CRT_SECURE_NO_WARNINGS 1 // needed to avoid wornings in MS VC++
#endif

#include<algorithm>
#include <stdarg.h>
#include <string.h> // for memcpy
#include <unistd.h>
#include <tiffio.h>
#include <fcntl.h> // for the libc "open" function see bug description in the SaveImageFP function.
#include<climits>
#include <ctime>
#include <cmath>
#include "common.h"


using namespace std;


const clock_t startTV = clock();
clock_t prevTV = startTV;

void prdn( const string & str ) {
  clock_t nowTV = clock();
  double start_elapsed = double( nowTV - startTV ) / CLOCKS_PER_SEC;
  double prev_elapsed = double( nowTV - prevTV ) / CLOCKS_PER_SEC;
  printf("DONE %s:  %f  %f\n", str.c_str(), prev_elapsed, start_elapsed);
  fflush(stdout);
  prevTV=nowTV;
}



/// \brief Constructor.
///
/// @param _terr Sets CtasErr::terr
/// @param mod   Sets CtasErr::module
/// @param msg   Sets CtasErr::message
///
CtasErr::CtasErr(ErrTp _terr, const string & mod, const string & msg)
  : terr(_terr)
  , module(mod)
  , message(msg)
{}

/// \brief Prints the error into the standard error stream.
void
CtasErr::report() const {
  switch ( terr ) {
  case WARN:
    cerr << "WARNING!";
    break;
  case ERR:
    cerr << "ERROR!";
    break;
  case FATAL:
    cerr << "FATAL ERROR!";
    break;
  }
  cerr << " In module \'" << module << "\'. " << message << endl;
  cerr.flush();
}


CtasErr::ErrTp
CtasErr::type() const {
  return terr;
}


void
throw_error(const string & mod, const string & msg) {
  CtasErr err(CtasErr::ERR, mod, msg);
  err.report();
  throw err;
}

void
throw_bug(const string & mod){
  throw_error(mod, "Must never happen. This is the bug, please report to developers.");
}

CtasErr
warn(const string & mod, const string & msg){
  CtasErr err(CtasErr::WARN, mod, msg);
  err.report();
  return err;
}

void
exit_on_error(const string & mod, const string & msg){
  CtasErr err(CtasErr::FATAL, mod, msg);
  err.report();
  cerr << "!!! EXITING ON ERROR !!!" << endl;
  exit(1);
}



#ifdef _WIN32
const string Path::DIRSEPARATOR = "\\";
#else
const string Path::DIRSEPARATOR = "/";
#endif

const Path Path::emptypath = Path();


/// Constructs the error which reports the unextractable element.
///
/// @param element The element whose extraction has failed.
///
void
Path::throw_unextracted(const string & element) const {
  throw_error
        ("class Path",
         "Could not extract " + element + " from path \"" + *this + "\".");
}

string
Path::drive () const {
#ifdef _WIN32
  char _drive[FILENAME_MAX];
  if ( _splitpath_s( c_str(), _drive, FILENAME_MAX, 0,0,0,0,0,0 ) )
        throw_unextracted("drive");
  return _drive;
#else
  return "";
#endif
}

string
Path::dir () const {
#ifdef _WIN32
  char _dir[FILENAME_MAX];
  if ( _splitpath_s( c_str(),0,0,_dir,FILENAME_MAX,0,0,0,0 ) )
        throw_unextracted("directory");
  return drive() + _dir;
#else
  string::size_type idx=this->rfind("/");
  return  idx == string::npos  ?  string("")  :  string(*this, 0, idx+1);
#endif
}

string
Path::name () const {
#ifdef _WIN32
  return title() + extension();
#else
  size_t found = this->rfind('/');
  if (found==string::npos)
    return *this;
  else
    return this->substr(found+1);
#endif
};


string
Path::title () const {
#ifdef _WIN32
  char _title[FILENAME_MAX];
  if ( _splitpath_s( c_str(), 0,0,0,0, _title, FILENAME_MAX, 0,0 ) )
        throw_unextracted("title");
  return _title;
#else
  string::size_type idx = name().rfind(".");
  if (idx==string::npos || idx == 0 )
        return name();
  else
        return name().substr(0, idx);
#endif
}

string
Path::dtitle () const {
  return dir() + title();
}


string
Path::extension () const {
#ifdef _WIN32
  char _ext[FILENAME_MAX];
  if ( _splitpath_s( c_str(), 0,0,0,0,0,0, _ext, FILENAME_MAX ) )
        throw_unextracted("extension");
  return _ext;
#else
  string::size_type
        dotidx = this->rfind("."),
        diridx = this->rfind("/");
  if ( dotidx != string::npos && ( diridx == string::npos || diridx+1 < dotidx ) )
        return this->substr(dotidx);
  else
        return "";
#endif
}



bool
Path::isdir() const {
  return ! empty() && name().empty();
}

Path &
Path::bedir() {
  if ( ! empty() && ! isdir() )
        *this += DIRSEPARATOR;
  return *this;
}


bool
Path::isabsolute() const {
#ifdef _WIN32
  return ! drive().empty();
#else
  return ! empty() && (*this)[0] == '/';
#endif
}


const Path Path::home() {
  char * hm;
  #ifdef _WIN32
  //return getenv("USERPROFILE");
  hm = getenv("APPDATA");
  #else
  hm = getenv("HOME");
  #endif
  return Path(hm).bedir();
}


Path
upgrade(const Path & path, const string & addthis) {
  return path.dir() + Path(addthis) + path.name();
}


Path
cdpath(const Path & dir, const Path & file){
  Path ddir = dir;
  ddir.bedir();

  if ( file.empty() )
        return ddir;
  else if ( file.isabsolute() || ddir.empty() )
        return file;
  else
      // Windows version is likely to fail here. See Path operator+.
        return ddir + file;
}


string
type_desc (Path*) {
  return "PATH";
}

int
_conversion (Path* _val, const string & in) {
  *_val = Path(in);
  return 1;
}


string
mask2format(const string & mask, int maxslice){
  string format(mask);
  // replace all '%' by "%%"
  string::size_type pos = format.find('%');
  while ( pos != string::npos ) {
    format.insert(pos, "%");
    pos = format.find('%', pos+2);
  }
  //replace last '@' with the format expression.
  format.replace( format.rfind('@'), 1,
                  "%0" + toString( toString(maxslice).length() ) + "u");
  return format;
}












const string
Contrast::modname="contrast";

/// \brief Constructor.
///
/// @param _cn Contrast type.
Contrast::Contrast(Cntype _cn)
  : _contrast(_cn) {}

/// \brief Constructor from name
///
/// @param _name Contrast name.
Contrast::Contrast(const string & _name) {
  string name = upper(_name);
  if ( name == "A" || name == "ABS" || name == "ABSORPTION" ) _contrast = ABS;
  else if ( name == "R" || name == "REF" || name == "REFRACTION" ) _contrast = REF;
  else if ( name == "P" || name == "PHS" || name == "PHASE" ) _contrast = PHS;
  else if ( name == "F" || name == "FLT" || name == "FILTERED" ) _contrast = FLT;
  else throw_error("contrast type", "The string \""+ _name +"\""
                                   " does not describe any known contrast.");
}

/// \brief Type of contrast
///
/// @return Type of contrast
///
Contrast::Cntype
Contrast::contrast() const {
  return _contrast;
}


/// \brief Name of the contrast.
///
/// @return Name of the contrast.
///
string
Contrast::name() const {
  switch (_contrast) {
  case REF:     return "REFRACTION";
  case ABS:     return "ABSORPTION";
  case PHS:     return "PHASE";
  case FLT:     return "FILTERED";
  default :
          throw_bug(__FUNCTION__);
          return "";
  }
}

const std::string Contrast::Desc =
            "Must be one of the following strings (case insensitive):\n"
            "A, ABS, ABSORPTION - for the absorption contrast\n"
            "R, REF, REFRACTION - for the refraction contrast\n"
            "P, PHS, PHASE      - for the phase contrast\n"
            "F, FLT, FILTERED   - for the pre-filtered contrast";



bool
operator==(const Contrast & a, const Contrast & b){
  return a.contrast() == b.contrast();
}


bool
operator!=(const Contrast & a, const Contrast & b){
  return a.contrast() != b.contrast();
}

string
type_desc (Contrast*){
  return "STRING";
}

int
_conversion (Contrast* _val, const string & in) {
  *_val=Contrast(in);
  return 1;
}


string
type_desc (Crop*){
  return "UINT:UINT:UINT:UINT";
}

int
_conversion (Crop* _val, const string & in) {
  int l, r, t, b;
  int scanres = sscanf( in.c_str(), "%i:%i:%i:%i", &t, &l, &b, &r);
  if (scanres != 4) // try , instead of :
    scanres = sscanf( in.c_str(), "%i,%i,%i,%i", &t, &l, &b, &r);
  if ( 4 != scanres || l<0 || r<0 || t<0 || b<0 )
    return -1;
  *_val = Crop(t, l, b, r);
  return 1;
}



void
crop(const Map & inarr, Map & outarr, const Crop & crp) {

  if ( ! crp.left && ! crp.right && ! crp.top && ! crp.bottom ) {
    outarr.reference(inarr);
    return;
  }
  if (  crp.left + crp.right >= inarr.shape()(1)  ||
        crp.top + crp.bottom >= inarr.shape()(0) ) {
    warn("Crop array", "Cropping (" + toString(crp) + ")"
         " is larger than array size ("+toString(inarr.shape())+")");
    return;
  }

  outarr.resize( inarr.shape()(0) - crp.top  - crp.bottom,
                 inarr.shape()(1) - crp.left - crp.right);
  outarr = inarr( blitz::Range(crp.top, inarr.shape()(0)-1-crp.bottom ),
                  blitz::Range(crp.left, inarr.shape()(1)-1-crp.right ) );

}

void
crop(Map & io_arr, const Crop & crp) {
  Map outarr;
  crop(io_arr, outarr, crp);
  if( io_arr.data() == outarr.data() )
    return;
  io_arr.resize(outarr.shape());
  io_arr=outarr.copy();
}




string
type_desc (Binn*) {
  return "UINT[:UINT]";
}

int
_conversion (Binn* _val, const string & in) {

  int x, y;

  if ( in.find_first_of(",:") !=  string::npos ) {

    int scanres = sscanf( in.c_str(), "%i:%i", &x, &y);
    if (scanres != 2) // try , instead of :
      scanres = sscanf( in.c_str(), "%i,%i", &x, &y);
    if ( 2 != scanres || x<1 || y<1 )
      return -1;

  } else {

    int xy;
    if ( 1 != sscanf( in.c_str(), "%i", &xy ) || xy<1 )
      return -1;
    x=xy;
    y=xy;

  }

  *_val = Binn(x, y);
  return 1;

}


void
binn(const Map & inarr, Map & outarr, const Binn & bnn) {

  if ( bnn.x == 1 && bnn.y == 1 ) {
    outarr.reference(inarr);
    return;
  }
  if ( ! bnn.x || ! bnn.y ) {
    warn("Binning aray", "Binning parameter cannot be zero.");
    return;
  }

  outarr.resize( inarr.shape()(0) / bnn.y , inarr.shape()(1) / bnn.x );

  for (blitz::MyIndexType ycur = 0 ; ycur < outarr.shape()(0) ; ycur++ )
    for (blitz::MyIndexType xcur = 0 ; xcur < outarr.shape()(1) ; xcur++ )
      outarr(ycur,xcur) = mean( inarr( blitz::Range(ycur*bnn.y, ycur*bnn.y+bnn.y-1),
                                         blitz::Range(xcur*bnn.x, xcur*bnn.x+bnn.x-1) ) );

}

void
binn(Map & io_arr, const Binn & bnn) {
  Map outarr;
  binn(io_arr, outarr, bnn);
  if( io_arr.data() == outarr.data() )
    return;
  io_arr.resize(outarr.shape());
  io_arr=outarr.copy();
}

const string BinnOptionDesc =
  "Binning factor(s).";




void rotate(const Map & inarr, Map & outarr, float angle, float bg) {

  const Shape sh = inarr.shape();

  if ( abs( remainder(angle, M_PI/2) ) < 1.0/max(sh(0),sh(1)) ) { // close to a 90-deg step

    const int nof90 = round(2*angle/M_PI);

    if ( ! (nof90%4)  ) { // 360deg
      outarr.reference(inarr);
    } else if ( ! (nof90%2) ) { //180deg
      outarr.resize(sh);
      outarr=inarr.copy().reverse(blitz::firstDim).reverse(blitz::secondDim);
    } else if (  ( nof90 > 0 && (nof90%3) ) || ( nof90 < 0 && ! (nof90%3) ) ) { // 270deg
      outarr.resize(sh(1),sh(0));
      outarr=inarr.copy().transpose(blitz::firstDim, blitz::secondDim).reverse(blitz::secondDim);
    } else { // 90deg
      outarr.resize(sh(1),sh(0));
      outarr=inarr.copy().transpose(blitz::firstDim, blitz::secondDim).reverse(blitz::firstDim);
    }

    return;

  }

  const float cosa = cos(-angle), sina = sin(-angle);
  const int
    rwidth = abs( sh(1)*cosa ) + abs( sh(0)*sina),
    rheight = abs( sh(1)*sina ) + abs( sh(0)*cosa);

  const Shape shf(rheight, rwidth);
  outarr.resize(shf);

  if ( ! isnormal(bg) ) {
    bg=0;
    bg += mean( inarr( blitz::Range::all(), 0 ) );
    bg += mean( inarr( 0, blitz::Range::all() ) );
    bg += mean( inarr( blitz::Range::all(), sh(1)-1 ) );
    bg += mean( inarr( sh(0)-1, blitz::Range::all() ) );
    bg /= 4.0;
  }

  const float
    constinx = ( sh(1) + rheight*sina - rwidth*cosa ) / 2.0,
    constiny = ( sh(0) - rwidth*sina - rheight*cosa ) / 2.0;

  for ( blitz::MyIndexType x=0 ; x < shf(1) ; x++) {
    for ( blitz::MyIndexType y=0 ; y < shf(0) ; y++) {

      const float xf = x*cosa - y*sina + constinx;
      const float yf = x*sina + y*cosa + constiny;
      const blitz::MyIndexType flx = floor(xf), fly = floor(yf);

      if ( flx < 1 || flx >= sh(1)-1 || fly < 1  || fly >= sh(0)-1 ) {
        outarr(y, x)=bg;
      } else {
        float v0 = inarr(fly,flx) + ( inarr(fly,flx+1) - inarr(fly,flx) ) * (xf-flx);
        float v1 = inarr(fly+1,flx) + ( inarr(fly+1,flx+1) - inarr(fly+1,flx) ) * (yf-fly);
        outarr(y, x) = v0 + (v1-v0) * (yf-fly);
      }

    }
  }

}

void
rotate(Map & io_arr, float angle, float bg) {
  Map outarr;
  rotate(io_arr, outarr, angle, bg);
  if( io_arr.data() == outarr.data() )
    return;
  io_arr.resize(outarr.shape());
  io_arr=outarr.copy();
}




const string CropOptionDesc =
  "top:left:bottom:right. Cropping from the edges of the image.";

const string CenterOptionDesc=
  "Deviation of the rotation axis from the center of the sinogram.\n"
  "Allows you to erase some minor artifacts in the reconstructed image"
  " which come from the inaccurate rotation position. In most real"
  " CT or TS experiments the rotation center of the sample"
  " is not exactly in the center of the recoded projection. Moreover,"
  " the rotation axis may be inclined from the vertical orientation."
  " If the inclination of the rotation axis is noticeable for your desired"
  " spatial resolution then you should rotate the images before the actual"
  " reconstruction. You can do it in the batch using \"mogrify\" command"
  " from the ImageMagick package with the -rotate option.";

const std::string IntOptionDesc =
  "If this option is not set, the output format defaults to the"
  " 32-bit float-point TIFF (regardless of the extension)."
  " If it is set, the image format is derived from the output"
  " file extension (TIFF if the extension does not correspond"
  " to any format).";

const std::string NeedForQuant =
  "The value is required only to produce the physically correct output."
  " If not set the results are qualitative rather than quantitative.";

const std::string ResolutionOptionDesc =
  "Size of the image pixel in microns. If not present, the program attempts to"
  " to calculate it from the image dpi." + NeedForQuant;

/// Lists all manual pages in the package. This string to be added to
/// every manual page.
const std::string SeeAlsoList =
  "ctas(1), ctas-bg(1), ctas-ct(1), ctas-ct-abs(1), ctas-ct-dei(1),"
  " ctas-ct-edei(1), ctas-ct-ipc(1),"
  " ctas-dei(1), ctas-edei(1), ctas-ipc(1), ctas-f2i(1), ctas-ff(1),"
  " ctas-sino(1), ctas-sino-abs(1), ctas-sino-dei(1), ctas-sino-ipc(1),"
  " ctas-ts(1), ctas-ct-line(1)";







/// \cond
struct ToLower{ char operator() (char c) const { return tolower(c); } };
struct ToUpper{ char operator() (char c) const { return toupper(c); } };
/// \endcond

string
upper(string str){
  transform (str.begin(), str.end(), str.begin(), ToUpper());
  return str;
}

string
lower(string str){
  transform (str.begin(), str.end(), str.begin(), ToLower());
  return str;
}


string
toString(const string fmt, ...){
  va_list ap;
  va_start(ap, fmt);
  const int MAXLINE = 4096;
  char buf[MAXLINE];
  vsnprintf(buf, MAXLINE, fmt.c_str(), ap);
  va_end(ap);
  return string(buf);
}









/// \brief Initializing constructor.
///
/// @param _showme Tells if the progress bar should be shown.
/// @param _message The description of the progress.
/// @param _steps Number of steps in the progress. If 0 then unknown.
///
ProgressBar::ProgressBar(bool _showme, const string & _message, int _steps) :
  showme(_showme), message(_message), steps(_steps) {

  if ( ! showme ) return;

  step = 0;
  waswidth = 0;
  reservedChs = 0;

  cout << "Starting process";
  if (steps) cout << " (" + toString(steps) + " steps)";
  cout << ": " << message << "." << endl;
  fflush(stdout);

  int nums = toString(steps).length();
  reservedChs = 14 + 2*nums;

  fmt = steps ?
        "%" + toString(nums) + "i of " + toString(steps) + " [%s] %4s" :
        string( "progress: %i" );

}

/// \brief Updates the progress bar.
///
/// @param curstep Current step. Advances +1 if zero.
///
void
ProgressBar::update(int curstep){

  if ( !showme || !reservedChs ) return; // Uninitialized progress bar.

  int progln = getwidth() - reservedChs;
  if ( progln <= 3 )  return; // if we have a very narrow terminal
  step = curstep ? curstep+1 : step + 1;

  if ( steps && step >= steps ) {
        done();
        return;
  }

  string outS;
  if (steps) {
    string eqs = string(progln*step/steps, '=') + string(progln, ' ') ;
    eqs.erase(progln);
    string prc = toString("%5.1f%% ", (100.0*step)/steps);
    outS = toString(fmt, step, eqs.c_str(), prc.c_str() );
  } else {
    outS = toString(fmt, step);
  }

  cout << string(waswidth+1, '\b') << outS;
  fflush(stdout);
  waswidth = outS.length();

}


void
ProgressBar::done(){

  if ( !showme || ! reservedChs ) return;

  int progln = getwidth() - reservedChs;
  if ( progln < 0 )  progln = 0; // if we have a very narrow terminal
  string eqs(progln, '=');

  cout << string(waswidth+1, '\r')
       << ( steps ?
            toString(fmt, steps, eqs.c_str(), "DONE. ") :
            toString(fmt, step) + " steps. DONE." )
       << endl
       << "Successfully finished " << message << "." << endl;
  fflush(stdout);

  reservedChs = 0;

}


#ifdef _WIN32
#  include <windows.h>
#else
#  include<termios.h>
#  include<sys/ioctl.h>
#endif

int
ProgressBar::getwidth(){
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE fd = GetStdHandle(STD_OUTPUT_HANDLE);
  if (fd == INVALID_HANDLE_VALUE)
    return 0;
  return GetConsoleScreenBufferInfo (fd, &info) ? info.dwSize.X - 1 : 0;
  //return (info.srWindow.Right - info.srWindow.Left + 1);
#else
  winsize size;
  return ( ioctl (STDOUT_FILENO, TIOCGWINSZ, &size ) < 0 ) ?  0 : size.ws_col - 1;
#endif
}










#ifdef OPENCL_FOUND

//#include<libconfig.h++>
//using namespace libconfig;

static bool clInited = false;

bool clIsInited() {

  if (clInited)
    return true;

  cl_int err;

  cl_uint nof_platforms;
  err = ::clGetPlatformIDs(0, 0, &nof_platforms);
  if (err != CL_SUCCESS) {
    warn("OpenCLinit", "Could not get number of OpenCL platforms: " + toString(err) );
    return false;
  }

  vector<cl_platform_id> platforms(nof_platforms);
  vector<cl_device_id> devices;

  err = clGetPlatformIDs(nof_platforms, platforms.data(), 0);
  if (err != CL_SUCCESS) {
    warn("OpenCLinit", "Could not get OpenCL platforms: " + toString(err) );
    return false;
  }

  for (int plidx=0; plidx < platforms.size() ; plidx++ ) {

    cl_uint nof_devices = 0;

    err = clGetDeviceIDs( platforms[plidx], CL_DEVICE_TYPE_GPU, 0, 0, &nof_devices);
    if (err != CL_SUCCESS) {
      warn("OpenCLinit", "Could not get OpenCL number of GPU devices of a platform: "
           + toString(err) );
    } else {

      vector<cl_device_id> platform_devices(nof_devices);
      err = clGetDeviceIDs( platforms[plidx], CL_DEVICE_TYPE_GPU,
                            nof_devices, platform_devices.data(), 0);
      if (err != CL_SUCCESS) {
        warn("OpenCLinit", "Could not get OpenCL GPU devices of a platform: "
             + toString(err) );
      } else {

        for (int devidx=0; devidx<platform_devices.size(); devidx++) {

          cl_device_id dev = platform_devices[devidx];
          bool errHappened=false;

          cl_bool devIsAvailable;
          err = clGetDeviceInfo(dev, CL_DEVICE_AVAILABLE,
                                sizeof(cl_bool), &devIsAvailable, 0);
          if (err != CL_SUCCESS) {
            warn("OpenCLinit", "Could not get OpenCL device info \"CL_DEVICE_AVAILABLE\": "
                 + toString(err) );
            errHappened=true;
          }

          cl_bool devCompilerIsAvailable;
          err = clGetDeviceInfo(dev, CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool),
                                &devCompilerIsAvailable, 0);
          if (err != CL_SUCCESS) {
            warn("OpenCLinit", "Could not get OpenCL device info \"CL_DEVICE_COMPILER_AVAILABLE\": "
                 + toString(err) );
            errHappened=true;
          }

          cl_device_exec_capabilities devExecCapabilities;
          err = clGetDeviceInfo(dev, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities),
                                &devExecCapabilities, 0);
          if (err != CL_SUCCESS) {
            warn("OpenCLinit", "Could not get OpenCL device info \"CL_DEVICE_EXECUTION_CAPABILITIES\": "
                 + toString(err) );
            errHappened=true;
          }

          if ( ! errHappened &&
               devIsAvailable &&
               devCompilerIsAvailable &&
               ( devExecCapabilities & CL_EXEC_KERNEL ) )
            devices.push_back(dev);

        }

      }

    }

  }


  if (devices.empty()) {

    warn("OpenCLinit", "No OpenCL devices found.");
    return false;

  } else { // more than one device found

    int idx=-1;
    cl_ulong devmem, devmaxmem=0;
    for (int devidx=0; devidx < devices.size(); devidx++) {
      err = clGetDeviceInfo(devices[devidx], CL_DEVICE_GLOBAL_MEM_SIZE,
                            sizeof(cl_ulong),  &devmem, 0);
      if (err == CL_SUCCESS  &&  devmem > devmaxmem) {
        devmaxmem = devmem;
        idx=devidx;
      }
    }

    if (idx >= 0)
      CL_device = devices[idx];

    /* TODO:
     * complete this part to read device name from the config
     * file instead of choosing the device with maximum memory
     * above.
     */

  }

  cl_platform_id platform;
  err = clGetDeviceInfo(CL_device, CL_DEVICE_PLATFORM, sizeof(cl_platform_id),  &platform, 0);
  if (err != CL_SUCCESS) {
    warn("OpenCLinit", "Could not get OpenCL device info \"CL_DEVICE_PLATFORM\": "
         + toString(err) );
    return false;
  }

  CL_context = clCreateContext(0, 1, &CL_device, 0, 0, &err);
  if (err != CL_SUCCESS) {
    warn("OpenCLinit", "Could not create OpenCL context: " + toString(err) );
    return false;
  }


//  cl_queue_properties queue_prop[] = {0};
//  CL_queue = clCreateCommandQueueWithProperties(CL_context, CL_device, 0, &err);
  CL_queue = clCreateCommandQueue(CL_context, CL_device, 0, &err);
  if (err != CL_SUCCESS) {
    warn("OpenCLinit", "Could not create OpenCL queue: " + toString(err) );
    return false;
  }

  clInited = true;
  return clInited;

}




cl_program initProgram(const char csrc[], size_t length, const string & modname) {

  if ( ! clIsInited() )
    return 0;

  const char * src = csrc;

  cl_int err;

  cl_program program =
    clCreateProgramWithSource( CL_context, 1, &src, &length, &err);
  if (err != CL_SUCCESS) {
    warn(modname, "Could not load OpenCL program: " + toString(err) );
    return 0;
  }

  err = clBuildProgram( program, 0, 0, "", 0, 0);
  if (err != CL_SUCCESS) {

    warn(modname, (string) "Could not build OpenCL program: " + toString(err) +
    ". More detailsd below:" );

    cl_build_status stat;
    err = clGetProgramBuildInfo(program, CL_device, CL_PROGRAM_BUILD_STATUS,
                                sizeof(cl_build_status), &stat, 0);
    if (err != CL_SUCCESS)
      warn(modname, "Could not get OpenCL program build status: " + toString(err) );
    else
      warn(modname, "   Build status: " + toString(stat));

    size_t len=0;
    err=clGetProgramBuildInfo(program, CL_device, CL_PROGRAM_BUILD_OPTIONS,
                              0, 0, &len);
    char * buildOptions = (char*) calloc(len, sizeof(char));
    if (buildOptions)
      err=clGetProgramBuildInfo(program, CL_device, CL_PROGRAM_BUILD_OPTIONS,
                                len, buildOptions, 0);
    if (err != CL_SUCCESS)
      warn(modname, "Could not get OpenCL program build options: " + toString(err) );
    else
      warn(modname, "   Build options: \"" + string(buildOptions, len) + "\"");
    if (buildOptions)
      free(buildOptions);

    err = clGetProgramBuildInfo(program, CL_device, CL_PROGRAM_BUILD_LOG,
                                0, 0, &len);
    char * buildLog = (char*) calloc(len, sizeof(char));
    if (buildLog)
      err = clGetProgramBuildInfo(program, CL_device, CL_PROGRAM_BUILD_LOG,
                                  len, buildLog, 0);
    if (err != CL_SUCCESS)
      warn(modname, "Could not get OpenCL program build log: " + toString(err) );
    else
      warn(modname, "   Build log:\n" + string(buildLog, len));
    if (buildLog)
      free(buildLog);

    warn(modname, "\n");

    return 0;

  }

  return program;

}


cl_kernel createKernel(cl_program program, const std::string & name) {
  cl_int clerr;
  cl_kernel kern = clCreateKernel ( program, name.c_str(), &clerr);
  if (clerr != CL_SUCCESS)
    throw_error("createKernel",
                "Could not create OpenCL kernel \"" + name + "\": " + toString(clerr));
  return kern;
}


std::string kernelName(cl_kernel kern) {
  if (!kern)
    throw_error("kernelName", "Invalid OpenCL kernel.");
  size_t len=0;
  cl_int clerr = clGetKernelInfo ( kern, CL_KERNEL_FUNCTION_NAME, 0, 0, &len);
  if ( clerr != CL_SUCCESS )
    throw_error("kernelName", "Could not get OpenCL kernel name size: " + toString(clerr));
  char *kernel_function = (char *) calloc(len, sizeof(char));
  std::string name;
  if (kernel_function) {
    clerr = clGetKernelInfo ( kern, CL_KERNEL_FUNCTION_NAME, len, kernel_function, 0);
    if (clerr == CL_SUCCESS)
      name = std::string(kernel_function, len);
    free(kernel_function);
    if (clerr != CL_SUCCESS)
      throw_error("kernelName", "Could not get OpenCL kernel name: " + toString(clerr));
  }
  return name;
}


cl_int execKernel(cl_kernel kern, size_t size) {
  cl_int clerr = clEnqueueNDRangeKernel( CL_queue, kern, 1, 0,  & size, 0, 0, 0, 0);
  if (clerr != CL_SUCCESS)
    throw_error("execKernel", "Failed to execute OpenCL kernel \"" + kernelName(kern) + "\": " + toString(clerr));
  clerr = clFinish(CL_queue);
  if (clerr != CL_SUCCESS)
    throw_error("execKernel", "Failed to finish OpenCL kernel \"" + kernelName(kern) + "\": " + toString(clerr));
  return clerr;
}




#endif // OPENCL_FOUND















#ifdef _WIN32
#  define STATIC_MAGICK
#  define MAGICK_STATIC_LINK
#endif
#define MAGICKCORE_QUANTUM_DEPTH 32
#define MAGICKCORE_HDRI_ENABLE 1
#include<Magick++.h>


/// \brief initializes image IO libraries
///
/// ImageMagick: allow reading up to 10k x 10k into memory, not HDD.
/// libTIFF: suppress warnings.
static bool
initImageIO(){

#ifdef MAGICKLIB_NAMESPACE
  using namespace MagickLib;
#else
  using namespace MagickCore;
#endif

  //MagickSizeType Msz = (numeric_limits<MagickSizeType>::max)();
  //SetMagickResourceLimit ( AreaResource , 10000 * 10000 * 4);
  //SetMagickResourceLimit ( FileResource , 1024 * 1024);
  //SetMagickResourceLimit ( DiskResource , Msz);
  //SetMagickResourceLimit ( MapResource , Msz);
  //SetMagickResourceLimit ( MemoryResource , Msz);

  // BUG in ImageMagick If I don'd do it here the TIFFSetWarningHandler
  // is called later in the code and causes
  // ../../magick/exception.c:1036: ThrowMagickExceptionList: Assertion `exception->signature == MagickSignature' failed.
  // whenever TIFFOpen is called
  try { Magick::Image imag; imag.ping("a.tif"); } catch (...) {}
  TIFFSetWarningHandler(0);

  return true;

}

static const bool imageIOinited = initImageIO();





float
PixelSize(const Path & filename) {
  static const float defaultSize = 1.0;
  Magick::Image imag;
  try { imag.ping(filename); }    catch ( Magick::WarningCoder err ) {}
  const Magick::Geometry dens = (Magick::Geometry) imag.density();
  float res = (float) dens.width();
  if ( ! dens.isValid() || ! res ) {
    warn("pixel size", "The resolution of the image \""+ filename+ "\"" "is invalid.");
    return defaultSize;
  }
  if ( res != dens.height() )
    warn("pixel size", "The resolutions of the image \""+ filename+ "\""
         "in vertical and horizontal directions differ.");

  switch ( imag.resolutionUnits() ) {
  case Magick::PixelsPerInchResolution :
    return 25400.0f / res;
  case Magick::PixelsPerCentimeterResolution :
    return 10000.0f / res;
  default:
    warn("pixel size", "Undefined resolution units of the image \""+ filename+ "\".");
    return defaultSize;
  }
}


Shape
ImageSizes(const Path & filename){
  Magick::Image imag;
  try {
    imag.ping(filename);
  }
  catch ( Magick::WarningCoder err ) {}
  catch ( Magick::Exception & error) {
    throw_error("get image size", "Could not read image file\""+filename+"\"."
                        " Caught Magick++ exception: \""+error.what()+"\".");
  }
  return Shape( imag.rows(), imag.columns() );
}


void
ImageSizes(const Path & filename, int *width, int *hight){
  Shape shp = ImageSizes(filename);
  if (width)
    *width = shp(1);
  if (hight)
    *hight = shp(0);
}


void
BadShape(const Path & filename, const Shape & shp){
  Shape ashp = ImageSizes(filename);
  if ( ashp != shp )
    throw_error("load image", "The shape of the image"
                "\"" + filename + "\"  (" + toString(ashp) + ") is not equal"
                " to the requested shape (" + toString(shp)  + ").");
}



/// Loads an image (lines) using TIFF library.
///
/// @param filename Name of the image
/// @param storage The array to store the image.
/// @param idxs The indexes of the line to read.
///        if empty then reads whole image.
///
static void
ReadImageLine_TIFF (const Path & filename, Map & storage,
                    const vector<int> & idxs ) {

  static const string modname = "load image tiff";

  TIFF *tif = TIFFOpen(filename.c_str(), "r");
  if( ! tif )
    throw CtasErr(CtasErr::WARN, modname,
                  "Could not read tif from file\"" + filename + "\".");

  uint32 width = 0, height = 0;
  uint16 spp = 0, bps = 0, fmt = 0, photo;

  if ( ! TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) )
    throw warn(modname,
               "Image \"" + filename + "\" has undefined width.");

  if ( ! TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) )
    throw warn(modname,
               "Image \"" + filename + "\" has undefined height.");

  if ( ! TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp) || spp != 1 )
    throw warn(modname,
               "Image \"" + filename + "\" has undefined samples per pixel"
               " or is not a grayscale.");
  if ( spp != 1 )
    throw warn(modname,
               "Image \"" + filename + "\" is not grayscale.");

  if ( ! TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps) )
    throw warn(modname,
               "Image \"" + filename + "\" has undefined bits per sample.");
  if ( bps != 8 && bps != 16 && bps != 32 )
    throw warn(modname,
               "Image \"" + filename + "\" has nonstandard " + toString(bps) +
               " bits per sample. Do not know how to handle it.");

  if ( ! TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &fmt) ) {
    string warnadd;
    if (bps != 32) {
      warnadd = toString(bps) +
           " bits per sample suggests unsigned integer format.";
      fmt = SAMPLEFORMAT_UINT;
    } else {
      warnadd = "32 bits per sample suggests float-point format.";
      fmt = SAMPLEFORMAT_IEEEFP;
    }
    // Gives to many warnings
    /*
    warn(modname,
         "Image \"" + filename + "\" has undefined sample format."
         " Guessing! " + warnadd);
    */
  }
  if ( fmt != SAMPLEFORMAT_UINT &&
       fmt != SAMPLEFORMAT_INT &&
       fmt != SAMPLEFORMAT_IEEEFP )
    throw warn(modname,
               "Image \"" + filename + "\" has unsupported sample format.");

  if ( ! TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photo) ||
       photo != PHOTOMETRIC_MINISBLACK )
    throw warn(modname,
         "Image \"" + filename + "\" has undefined or unsupported"
         " photometric interpretation.");

  const int readheight = idxs.size() ? idxs.size() : height;
  storage.resize(readheight,width);

  tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

  for (uint curidx = 0; curidx < readheight; curidx++) {

    uint32 row = idxs.size() ? idxs[curidx] : curidx;

    if ( row >= height || row < 0 ) {

      warn("load imagelines tiff",
      "The index of the line to be read (" + toString(row) + ")"
      " is outside the image boundaries (" + toString(height) + ").");

      storage(curidx, blitz::Range::all()) = 0.0;

    } else {

      if ( TIFFReadScanline(tif, buf, row) < 0 ) {
        _TIFFfree(buf);
        throw warn(modname,
                   "Failed to read line " + toString(row) +
                   " in image \"" + filename + "\".");
      }

      #define blitzArrayFromData(type) \
        blitz::Array<type,1> ( (type *) buf, \
                               blitz::shape(width), \
                               blitz::neverDeleteData)

      switch (fmt) {
      case SAMPLEFORMAT_UINT :
        if (bps==8)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(uint8);
        else if (bps==16)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(uint16);
        else if (bps==32)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(uint32);
        break;
      case SAMPLEFORMAT_INT :
        if (bps==8)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(int8);
        else if (bps==16)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(int16);
        else if (bps==32)
          storage(curidx, blitz::Range::all()) = 1.0 * blitzArrayFromData(int32);
        break;
      case SAMPLEFORMAT_IEEEFP :
        storage(curidx, blitz::Range::all()) = blitzArrayFromData(float);
        break;
      }

    }

  }


  _TIFFfree(buf);
  TIFFClose(tif);

}

/// Loads an image (lines) using TIFF library.
///
/// @param filename Name of the image
/// @param storage The array to store the image.
/// @param idxs The indexes of the line to read.
///        if empty then reads whole image.
///
inline static void
ReadImageLine_TIFF (const Path & filename, Line & storage, int idx) {
  Map _storage;
  ReadImageLine_TIFF(filename, _storage, vector<int>(1,idx) );
  storage=_storage(0,blitz::Range::all());
}


/// Loads an image using ImageMagick library.
///
/// @param filename Name of the image
/// @param storage The array to store the image.
///
inline static void
ReadImage_TIFF (const Path & filename, Map & storage) {
  ReadImageLine_TIFF(filename, storage, vector<int>() );
}




/// Loads an image using ImageMagick library.
///
/// @param filename Name of the image
/// @param storage The array to store the image.
///
static void
ReadImage_IM (const Path & filename, Map & storage ){

  Magick::Image imag;
  try { imag.read(filename); }
  catch ( Magick::WarningCoder err ) {}
  catch ( Magick::Exception & error) {
    throw_error("load image IM", "Could not read image file\""+filename+"\"."
                " Caught Magick++ exception: \""+error.what()+"\".");
  }
  if ( imag.type() != Magick::GrayscaleType )
    warn("load image IM",
         "Input image \"" + filename + "\" is not grayscale.");

  const int
    width = imag.columns(),
    hight = imag.rows();
  storage.resize( hight, width );

  // below might be buggy - see notes in SaveImageINT_IM
  /*
  const Magick::PixelPacket
    * pixels = imag.getConstPixels(0,0,width,hight);
  float * data = storage.data();
  for ( int k = 0 ; k < hight*width ; k++ )
    *data++ = (float) Magick::ColorGray( *pixels++  ) .shade();
  */

  /* Replacement for the buggy block */
  for (blitz::MyIndexType curw = 0 ; curw < width ; curw++)
    for (blitz::MyIndexType curh = 0 ; curh < hight ; curh++)
      storage(curh,curw) = Magick::ColorGray(imag.pixelColor(curw, curh)).shade();
  /* end replacement */

}


void
ReadImage (const Path & filename, Map & storage ){
  try { ReadImage_TIFF(filename, storage); }
  catch (CtasErr err) {
    if (err.type() != CtasErr::WARN)
      throw;
    ReadImage_IM(filename, storage);
  }
}


void
ReadImage(const Path & filename, Map & storage, const Shape & shp){
  BadShape(filename, shp);
  ReadImage(filename, storage);
}








/// \brief Reads one line of the image using ImageMagick library.
///
/// @param filename The name of the file with the image.
/// @param storage Line to read into.
/// @param idx The index of the line to read.
///
static void
ReadImageLine_IM (const Path & filename, Line & storage, int idx){

  Magick::Image imag;
  try { imag.read(filename); } catch ( Magick::WarningCoder err ) {}
  if ( imag.type() != Magick::GrayscaleType )
    warn("load imageline IM",
         "Input image \"" + filename + "\" is not grayscale.");

  const int width = imag.columns();
  if ( idx < 0 || (unsigned) idx >= imag.rows() )
    throw_error("load imageline IM",
                "The index of the line to be read (" + toString(idx) + ")"
                " is outside the image boundaries (" + toString(imag.rows()) + ").");
  storage.resize(width);

  // below might be buggy - see notes in SaveImageINT_IM
  /*
  const Magick::PixelPacket
    * pixels = imag.getConstPixels(0,idx,width,1);
  float * data = storage.data();
  for ( int k = 0 ; k < width ; k++ )
    *data++ = (float) Magick::ColorGray( *pixels++  ) .shade();
  */

  /* Replacement for the buggy block */
  for (blitz::MyIndexType curw = 0 ; curw < width ; curw++)
    storage(curw) = Magick::ColorGray(imag.pixelColor(curw, idx)).shade();
  /* end replacement */

}


void
ReadImageLine (const Path & filename, Line & storage, int idx) {
  try { ReadImageLine_TIFF(filename, storage, idx); }
  catch (CtasErr err) {
    if (err.type() != CtasErr::WARN)
      throw;
    ReadImageLine_IM(filename, storage, idx);
  }
}


void
ReadImageLine(const Path & filename, Line & storage, int idx,
              const Shape &shp) {
  BadShape(filename, shp);
  ReadImageLine(filename, storage, idx);
}




/// \brief Reads many line of the image using ImageMagick library.
///
/// @param filename The name of the file with the image.
/// @param storage Array to read into.
/// @param idxs The indexes of the line to read.
///
static void
ReadImageLine_IM (const Path & filename, Map & storage,
                  const vector<int> &idxs) {

  Magick::Image imag;
  try { imag.read(filename); } catch ( Magick::WarningCoder err ) {}
  if ( imag.type() != Magick::GrayscaleType )
    warn("load imagelines IM",
                 "Input image \"" + filename + "\" is not grayscale.");

  const int width = imag.columns();
  const int hight = imag.rows();
  const int readheight = idxs.size() ? idxs.size() : hight;
  storage.resize( readheight, width );

  for ( blitz::MyIndexType curel = 0 ; curel < readheight ; curel++ ){
    int cursl = idxs.size() ? idxs[curel] : curel;
    if ( cursl >= hight ) {
      warn("load imagelines IM",
           "The index of the line to be read (" + toString(cursl) + ")"
           " is outside the image boundaries (" + toString(hight) + ").");
      storage(curel, blitz::Range::all() ) = 0.0;
    } else {
      // below might be buggy - see notes in SaveImageINT_IM
      /*
      const Magick::PixelPacket *pixels = imag.getConstPixels(0,cursl,width,1);
      for ( blitz::MyIndexType k = 0 ; k < width ; k++ )
        storage( (blitz::MyIndexType) curel, k) =
          (float) Magick::ColorGray( *pixels++  ) .shade();
      */
      /* Replacement for the buggy block */
      for (blitz::MyIndexType curw = 0 ; curw < width ; curw++)
        storage(curel, curw) = Magick::ColorGray(imag.pixelColor(curw, cursl)).shade();
      /* end replacement */

    }

  }

}



void
ReadImageLine (const Path & filename, Map & storage,
               const vector<int> & idxs) {

  if ( ! idxs.size() ) {
    ReadImage(filename, storage);
    return;
  }

  try { ReadImageLine_TIFF(filename, storage, idxs); }
  catch (CtasErr err) {
    if (err.type() != CtasErr::WARN)
      throw;
      ReadImageLine_IM(filename, storage, idxs);
  }

}



void
ReadImageLine(const Path & filename, Map & storage,
                          const vector<int> & idxs, const Shape & shp){
  BadShape(filename, shp);
  ReadImageLine(filename, storage, idxs);
}







/// Saves image in integer format using ImageMagick library.
///
/// @param filename file to save image into.
/// @param storage array with the image.
///
static void
SaveImageINT_IM (const Path & filename, const Map & storage){

  if ( ! storage.size() ) {
    warn("save image", "Zero-sized array for image.");
    return;
  }

  const int
    width = storage.columns(),
    hight = storage.rows();

  Map _storage;
  if ( storage.isStorageContiguous()  &&  storage.stride() == Shape(width,1) )
    _storage.reference(storage);
  else {
    _storage.resize(storage.shape());
    _storage = storage;
  }

  //Magick::Image imag( Magick::Geometry(width, hight), "black" );
  Magick::Image imag( width, hight, "I", Magick::FloatPixel, _storage.data() );
  imag.classType(Magick::DirectClass);
  imag.type( Magick::GrayscaleType );
  imag.depth(8);
  imag.magick("TIFF"); // saves to tif if not overwritten by the extension.
<<<<<<< HEAD

  /*
  Magick::Pixels view(imag);
  Magick::Quantum * pixels = view.get(0,0,width,hight);
  Magick::ColorGray colg;
=======
>>>>>>> 4dc2851306592e2bfb2ff7447718df1c11102daf

/*
  //Magick::Pixels view(imag);
  //Magick::Quantum * pixels = view.get(0,0,width,hight);
  imag.modifyImage();
  Magick::PixelPacket * pixels = imag.getPixels(0,0,width,hight);
  Magick::ColorGray colg;
  const float *data = _storage.data();
<<<<<<< HEAD
  for ( int k = 0 ; k < hight*width ; k++ )
    *pixels++ = QuantumRange * *data++ ;
  view.sync();
  */
=======
  for ( int k = 0 ; k < hight*width ; k++ ) {
    //*pixels++ = QuantumRange * *data++ ;
    *pixels++ = Magick::PixelPacket( ( colg.shade( *data++ ), colg ) );
  }
  //view.sync();
  imag.syncPixels();
*/
>>>>>>> 4dc2851306592e2bfb2ff7447718df1c11102daf

  try { imag.write(filename); }
  catch ( Magick::Exception & error) {
    throw_error("save image IM", "Could not write image file\""+filename+"\"."
                        " Caught Magick++ exception: \""+error.what()+"\".");
  }




}




namespace blitz {

/// Limits the number to the [0;1] region
///
/// @param x number to be limited
///
/// @return limited number
///
float
limit01(float x){
  return ( x < 0.0 ) ?
    0.0 :
    ( x > 1.0 ? 1.0 : x ) ;
}

/// \cond
BZ_DECLARE_FUNCTION(limit01);
/// \endcond

}




#ifdef OPENCL_FOUND

char limit_array_src[] = {
  #include "limit.cl.includeme"
};

bool limit_array_inited=false;
cl_program limit_array_cl_program=0;
cl_kernel limit_array_cl_kernel=0;

bool init_limit_array_cl() {

  if (limit_array_inited)
    return limit_array_cl_program && limit_array_cl_kernel;

  limit_array_cl_program =
    initProgram( limit_array_src, sizeof(limit_array_src), "Limit array" );

  if ( limit_array_cl_program )
    limit_array_cl_kernel = createKernel(limit_array_cl_program, "limit_array");

  limit_array_inited = true;
  return limit_array_cl_program && limit_array_cl_kernel;

}

#endif  // OPENCL_FOUND


/// \brief Save the array into integer image.
///
/// Stores the array in the integer-based image. If minval is equal to maxval
/// then the minimum and maximum values of the array data corresponds to black
/// and white respectively.
///
/// @param filename the name of the image file image.
/// @param storage the array to be written to the image.
/// @param minval the value corresponding to black.
/// @param maxval the value corresponding to white.
///
static void
SaveImageINT (const Path &filename, const Map &storage,
              float minval=0, float maxval=0 ) {

  if ( ! storage.size() ) {
    warn("save image",
         "Zero-sized array for image '" + filename + "': won't save." );
    return;
  }

  const int
    width = storage.columns(),
    hight = storage.rows();

  Map _storage;
  if ( storage.isStorageContiguous()  &&  storage.stride() == Shape(width,1) )
    _storage.reference(storage);
  else {
    _storage.resize(storage.shape());
    _storage = storage;
  }

  Map stor(_storage.shape());
  if (minval == maxval) {
    minval = (blitz::min)(_storage);
    maxval = (blitz::max)(_storage);
  }
  if (minval == maxval) {

    warn("save image",
         "All elements in the image '" + filename + "' have the same value.");
    if      ( minval < 0.0 ) stor = 0.0;
    else if ( minval > 1.0 ) stor = 1.0;
    else                     stor = minval;

  } else {

    const string modname = "Limit array";

// #ifdef CHECK_IF_CPU_IS_FASTER
#ifdef OPENCL_FOUND
    if ( init_limit_array_cl() ) {

      cl_int err;
      cl_mem clStorage = 0;

      clStorage = blitz2cl(_storage, CL_MEM_READ_WRITE);
      setArg(limit_array_cl_kernel, 0, clStorage);
      setArg(limit_array_cl_kernel, 1, minval);
      setArg(limit_array_cl_kernel, 2, maxval);
      execKernel(limit_array_cl_kernel, _storage.size());
      cl2blitz(clStorage, stor);
      if (clStorage)
        clReleaseMemObject(clStorage);

    } else {
#endif  // OPENCL_FOUND
// #endif  // CHECK_IF_CPU_IS_FASTER

      stor = ( _storage - minval ) / (maxval-minval);
      stor = limit01(stor);

// #ifdef CHECK_IF_CPU_IS_FASTER
#ifdef  OPENCL_FOUND
    }
#endif  // OPENCL_FOUND
// #endif  // CHECK_IF_CPU_IS_FASTER

  }
  SaveImageINT_IM(filename, stor);

}




/// \brief Save the array into float point TIFF image.
///
/// Stores the array in the float-point TIFF file. Be careful: limited number
/// of editors/viewers/analyzers support float point format.
///
/// @param filename the name of the image file image.
/// @param storage the array to be written to the image.
///
void
SaveImageFP (const Path & filename, const Map & storage){

  if ( ! storage.size() ) {
    warn("save image", "Zero-sized array for image.");
    return;
  }

  const int
    width = storage.columns(),
    hight = storage.rows();

  Map _storage;
  if ( storage.isStorageContiguous()  &&  storage.stride() == Shape(width,1) )
    _storage.reference(storage);
  else {
    _storage.resize(storage.shape());
    _storage = storage;
  }

  // BUG in libtiff
  // On platforms (f.e. CentOS) the TIFFOpen function fails,
  // while TIFFFdOpen works well. On the MS Windows the
  // TIFFFdOpen does not work, while TIFFOpen does.

  int fd=0;
#ifdef _WIN32
  TIFF *image = TIFFOpen(filename.c_str(), "w");
#else
  fd = open (filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd < 1)
        throw_error("save float-point image",
                                "Could not open file \"" + filename + "\" for writing.");
  TIFF *image = TIFFFdOpen(fd, filename.c_str(), "w");
#endif

  if( ! image ) {
        if (fd) close(fd);
        throw_error("save float-point image",
                                "Could create tif from file\"" + filename + "\".");
  }

  // We need to set some values for basic tags before we can add any data
  TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(image, TIFFTAG_IMAGELENGTH, hight);
  TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 32);
  TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, hight);
  TIFFSetField(image, TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_IEEEFP);
  TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

  int wret = TIFFWriteRawStrip(image, 0, (void*) _storage.data(), width*hight*4);
  TIFFClose(image);
  if (fd) close(fd);
  if ( -1 == wret )
        throw_error("save 32-bit image",
                                "Could not save image to file \"" + filename + "\".");

}


void
SaveImage(const Path & filename, const Map & storage, bool saveint){
  if (saveint) SaveImageINT(filename, storage);
  else SaveImageFP(filename, storage);
}

void
SaveImage(const Path & filename, const Map & storage,
          float minval, float maxval ){
  SaveImageINT(filename, storage, minval, maxval);
}
















void
SaveData ( const Path filename, ... ) {

  va_list ap;
  va_start(ap, filename);

  vector<const Line*> storage;
  while ( const Line *curstor = va_arg(ap,const Line*) )
    storage.push_back(curstor);
  va_end(ap);

  int nof_args = storage.size();
  if ( ! nof_args ) {
    warn("write data", "No arrays provided for output. Nothing to do." );
    return;
  }

  int size = storage.front()->size();
  if ( ! size ) {
    warn("write data", "Empty arrays provided for output. Nothing to do." );
    return;
  }
  for (int icur = 0 ; icur < nof_args ; icur++)
    if ( storage[icur]->size() != size )
      throw_error("write data",
                  "The size of the array in the position " + toString(icur) +
                                  " (" + toString(storage[icur]->size()) + ") does not match the"
                                  " size of the first array (" + toString(size) + ").");

  FILE *funcf = fopen( filename.c_str(), "w" );
  if ( ! funcf )
    throw_error("write data", "Could not open output file \"" + filename + "\".");

  for (int element = 0 ; element < size ; element++) {
    bool printok = true;
    int curarray = -1;
    while (printok && ++curarray < nof_args)
      printok = fprintf( funcf, "%G ", (*storage[curarray])(element) ) >= 0;
    if ( ! printok ||
                 fseek (funcf, -1, SEEK_CUR) || // removes last space
                 fprintf(funcf, "\n") < 0 ) {
      fclose (funcf);
      throw_error("write data", "Could not print into output file"
                                  " \"" + filename + "\" in position " + toString(element) + ".");
    }
  }

  fclose (funcf);

}


void
LoadData ( const Path filename, ... ) {

  vector<Line*> storage;

  va_list ap;
  va_start(ap, filename);
  while ( Line * curstor = va_arg(ap, Line*) )
    storage.push_back( curstor );
  va_end(ap);

  int nof_args = storage.size();
  if ( ! nof_args ) {
    warn("read data", "No arrays provided for input. Nothing to do." );
    return;
  }
  vector< vector<float> > data_read(nof_args);

  FILE *funcf = fopen( filename.c_str(), "r" );
  if ( ! funcf )
    throw_error("read data", "Could not open input file \"" + filename + "\".");

  bool this_is_the_end = false;
  while ( ! this_is_the_end ) {
    for ( int curarray = 0 ; curarray < nof_args ; curarray++ ) {
      float toread;
      if ( fscanf( funcf, "%f ", &toread ) != 1 ) {
        fclose (funcf);
        throw_error("read data", "Could not scan float from input file"
                                        " \"" + filename + "\" at line " +
                                        toString(data_read[curarray].size() + 1) + ", position " +
                                        toString(curarray + 1) + ".");
      }
      data_read[curarray].push_back(toread);
    }
    this_is_the_end = fscanf(funcf, "\n") < 0 || feof(funcf);
  }

  fclose (funcf);

  int size = data_read[0].size();
  for ( int curarray = 0 ; curarray < nof_args ; curarray++ ) {
    (*storage[curarray]).resize(size);
    for (int element = 0 ; element < size ; element++)
      (*storage[curarray])(element) = data_read[curarray][element];
  }

}
