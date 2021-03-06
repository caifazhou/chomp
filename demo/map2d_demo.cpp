/*
* Copyright (c) 2008-2014, Matt Zucker
*
* This file is provided under the following "BSD-style" License:
*
* Redistribution and use in source and binary forms, with or
* without modification, are permitted provided that the following
* conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
* USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "Map2D.h"
#include <png.h>
#include <getopt.h>
#include "MotionOptimizer.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace mopt;

#ifdef MZ_HAVE_CAIRO
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#endif

// Utility function to visualize maps
bool savePNG_RGB24(const std::string& filename,
                   size_t ncols, size_t nrows, 
                   size_t rowsz,
                   const unsigned char* src,
                   bool yflip=false) {

  FILE* fp = fopen(filename.c_str(), "wb");
  if (!fp) { 
    std::cerr << "couldn't open " << filename << " for output!\n";
    return false;
  }
  
  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr) {
    std::cerr << "error creating png write struct\n";
    return false;
  }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    std::cerr << "error creating png info struct\n";
    png_destroy_write_struct(&png_ptr,
			     (png_infopp)NULL);
    fclose(fp);
    return false;
  }  

  if (setjmp(png_jmpbuf(png_ptr))) {
    std::cerr << "error in png processing\n";
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, 
	       ncols, nrows,
	       8, 
	       PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  std::vector<unsigned char> buf(ncols*4);

  const unsigned char* rowptr;
  if (yflip) {
    rowptr = src + rowsz * (nrows-1);
  } else {
    rowptr = src;
  }

  for (size_t y=0; y<nrows; ++y) {
    const unsigned char* pxptr = rowptr;
    buf.clear(); 
    for (size_t x=0; x<ncols; ++x) {
      buf.push_back(pxptr[2]);
      buf.push_back(pxptr[1]);
      buf.push_back(pxptr[0]);
      pxptr += 4;
    }
    png_write_row(png_ptr, (png_bytep)&(buf[0]));
    if (yflip) {
      rowptr -= rowsz;
    } else {
      rowptr += rowsz;
    }
  }

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  return true;

}

//////////////////////////////////////////////////////////////////////
// function to help evaluate collisons for gradients

class MapCollisionFunction : public CollisionFunction {

  private:
    Map2D * map;

  public:
    MapCollisionFunction( size_t cspace_dofs,
                          size_t workspace_dofs, 
                          size_t n_bodies,
                          double gamma,
                          Map2D * map ) :
        CollisionFunction(cspace_dofs, workspace_dofs, n_bodies, gamma ),
        map( map )
    {}

    virtual double getCost(const MatX& q, size_t body_index,
                           MatX& dx_dq, MatX& cgrad ) 
    {
        assert( (q.rows() == 2 && q.cols() == 1) ||
                (q.rows() == 1 && q.cols() == 2) );

        dx_dq.resize(3, 2);
        dx_dq.setZero();

        dx_dq << 1, 0, 0, 1, 0, 0;

        cgrad.resize(3, 1);

        vec3f g;
        
        float c = map->sampleCost(vec3f(q(0), q(1), 0.0), g);

        cgrad << g[0], g[1], 0.0;

        return c;
    }
};

//////////////////////////////////////////////////////////////////////
// help generate an an initial trajectory

void generateInitialTraj(MotionOptimizer & chomper,
                         int N, 
                         const vec2f& p0, 
                         const vec2f& p1,
                         MatX& q0,
                         MatX& q1) {
  
    q0.resize(1, 2);
    q1.resize(1, 2);

    q0 << p0.x(), p0.y();
    q1 << p1.x(), p1.y();

    chomper.getTrajectory().initialize( q0, q1, N );
  
}


//////////////////////////////////////////////////////////////////////
// helper class to visualize stuff

#ifdef MZ_HAVE_CAIRO

class PdfEmitter: public DebugObserver {
public:

  const Map2D& map;
  const MatX xi_init;
  
  int dump_every;
  int count;

  const char* filename;
  
  cairo_surface_t* surface;
  cairo_surface_t* image;
  cairo_t* cr;
  int width, height;
  float mscl;

  std::vector<unsigned char> mapbuf;

  std::ostringstream ostream;
  bool dump_data_to_file;
  
  PdfEmitter(const Map2D& m, const MatX& x, int de,
             const char* f, bool dump):
    map(m), xi_init(x), dump_every(de), count(0), filename(f),
    dump_data_to_file( dump ) 
  {
      
    ostream << filename << " {";
    
    Box3f bbox = map.grid.bbox();
    vec3f dims = bbox.p1 - bbox.p0;

    mscl = (400) / std::max(dims.x(), dims.y());
    
    width = int(mscl * dims.x());
    height = int(mscl * dims.y());
    
    surface = cairo_pdf_surface_create(filename, width, height);
    
    cr = cairo_create(surface);
    
    size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, map.grid.nx());

    map.rasterize(Map2D::RASTER_OCCUPANCY, mapbuf, stride);
    
    image = cairo_image_surface_create_for_data(&(mapbuf[0]), 
                                                CAIRO_FORMAT_RGB24,
                                                map.grid.nx(), map.grid.ny(),
                                                stride);
  }

  virtual ~PdfEmitter() {

    cairo_surface_destroy(surface);
    cairo_surface_destroy(image);
    cairo_destroy(cr);

  }

  void appendInfoToFile( const std::string & filename )
  {
      if (dump_data_to_file && filename.size() > 0 ){
          std::ofstream myfile;
          myfile.open (filename.c_str(), std::ios::app );
          myfile  << ostream.str() << "}\n";

          myfile.close();
      }
  }

  virtual int notify(const OptimizerBase& chomper, 
                     EventType event,
                     size_t iter,
                     double curObjective,
                     double lastObjective,
                     double hmag)
  {
    
    if ( dump_data_to_file ){
        ostream << "[" << chomper.problem.getTimesString()
                << ", iter:" << iter 
                << ", objective:" << curObjective
                <<  "], ";
    }else {
        DebugObserver::notify(chomper, event, iter, 
                                   curObjective, lastObjective, hmag);
    }
    
    if ( dump_every < 0 ) { return 0; }

    if ( !( (event == FINISH) ||
            (event == INIT )  ||
            (dump_every > 0 && iter % dump_every == 0 ) ) ) {
        return 0;
    }

    
    if (count++) {
      cairo_show_page(cr);
    }
    
    float cs = map.grid.cellSize();
    Box3f bbox = map.grid.bbox();

    cairo_identity_matrix(cr);
    cairo_translate(cr, 0, height);
    cairo_scale(cr, mscl, -mscl);
    cairo_translate(cr, -bbox.p0.x(), -bbox.p0.y());

    cairo_save(cr);
    cairo_scale(cr, cs, cs);
    cairo_set_source_surface(cr, image, bbox.p0.x()/cs, bbox.p0.y()/cs);
    cairo_paint(cr);
    cairo_restore(cr);
  
    cairo_set_line_width(cr, 1.0*cs);
    cairo_set_source_rgb(cr, 0.0, 0.1, 0.5);

    const Trajectory & trajectory = chomper.problem.getTrajectory();
    int N = trajectory.N();

    for (int i=0; i<N; ++i) {
      vec3f pi(xi_init(i,0), xi_init(i,1), 0.0);
      cairo_arc(cr, pi.x(), pi.y(), 1.0*cs, 0.0, 2*M_PI);
      cairo_fill(cr);
    }

    const MatX& q0 = trajectory.getQ0();
    const MatX& q1 = trajectory.getQ1();

    cairo_set_source_rgb(cr, 0.5, 0.0, 1.0);
    cairo_arc(cr, q0(0), q0(1), 4*cs, 0.0, 2*M_PI);
    cairo_fill(cr);
    cairo_arc(cr, q1(0), q1(1), 4*cs, 0.0, 2*M_PI);
    cairo_fill(cr);

    for (int i=0; i<N; ++i) {
      vec3f pi(trajectory(i,0), trajectory(i,1), 0.0);
      cairo_arc(cr, pi.x(), pi.y(), 2*cs, 0.0, 2*M_PI);
      cairo_fill(cr);
    }
    

    return 0;

 }

  
};

#endif

//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////

void usage(int status) {
  std::ostream& ostr = status ? std::cerr : std::cout;
  ostr <<
    "usage: map2d_demo OPTIONS map.txt\n"
    "Also, checkout the map2d_tests.sh script!\n"
    "\n"
    "OPTIONS:\n"
    "\n"
    "  -l, --algorithm          Algorithm used for optimization\n"
    
    "  -c, --coords             Set start, goal (x0,y0,x1,y1)\n"
    "  -n, --num                Number of steps for trajectory\n"
    "  -a, --alpha              Overall step size for CHOMP\n"
    "  -g, --gamma              Step size for collisions\n"
    "  -m, --max-iter           Set maximum iterations\n"
    "  -e, --error-tol          Relative error tolerance\n"
    "  -p, --pdf                Output PDF's\n"
    "  -k, --covariance         Do covariant optimization\n"
    "  -d, --dump               Dump recorded data to a given filename\n"
    "  -o, --objective          Quantity to minimize (vel|accel)\n"
    "  -b, --bounds             Bound the trajectory to the given area\n"
    "  -C, --coll_constraint    Treat collisions as a constraint\n"
    "      --help               See this message.\n";
  exit(status);
}

int main(int argc, char** argv) {

  const struct option long_options[] = {
    { "algorithm",         required_argument, 0, 'l' },
    { "coords",            required_argument, 0, 'c' },
    { "num",               required_argument, 0, 'n' },
    { "alpha",             required_argument, 0, 'a' },
    { "gamma",             required_argument, 0, 'g' },
    { "error-tol",         required_argument, 0, 'e' },
    { "max-iter",          required_argument, 0, 'm' },
    { "objective",         required_argument, 0, 'o' },
    { "pdf",               required_argument, 0, 'p' },
    { "dump",              required_argument, 0, 'd' },
    { "covariance",        no_argument,       0, 'k' },
    { "coll_constriant",   no_argument,       0, 'C' },
    { "help",              no_argument,       0, 'h' },
    { "bounds",            no_argument,       0, 'b' },
    { 0,                   0,                 0,  0  }
  };

  const char* short_options = "l:c:n:a:g:e:m:o:p:d:kChb";
  int opt, option_index;

  bool do_covariant = false;
  int N = 127;
  double gamma = 0.5;
  double alpha = 0.02;
  double errorTol = 1e-6;
  size_t max_iter = 500;
  ObjectiveType otype = MINIMIZE_VELOCITY;
  OptimizationAlgorithm alg = NONE;
  float x0=0, y0=0, x1=0, y1=0;
  int doPDF = -2;
  bool doBounds = false;
  bool collision_constraint = false;

  std::string filename;
  bool dump_data;

  while ( (opt = getopt_long(argc, argv, short_options, 
                             long_options, &option_index) ) != -1 ) {

    switch (opt) {
    case 'l':
      alg = algorithmFromString( optarg );
      break;
    case 'k':
      do_covariant = true;
      break;
    case 'c': 
      if (sscanf(optarg, "%f,%f,%f,%f", &x0, &y0, &x1, &y1) != 4) {
        std::cerr << "error parsing coords!\n\n";
        usage(1);
      } 
      break;
    case 'n':
      N = atoi(optarg);
      break;
    case 'a': 
      alpha = atof(optarg);
      break;
    case 'g':
      gamma = atof(optarg);
      break;
    case 'e':
      errorTol = atof(optarg);
      break;
    case 'm':
      max_iter = atoi(optarg);
      break;
    case 'o':
      if (!strcasecmp(optarg, "vel")) {
        otype = MINIMIZE_VELOCITY;
      } else if (!strcasecmp(optarg, "accel")) {
        otype = MINIMIZE_ACCELERATION;
      } else {
        std::cerr << "error parsing opt. type: " << optarg << "\n\n";
        usage(1);
      }
      break;
    case 'p':
      doPDF = atoi( optarg );
      break;
    case 'h':
      usage(0);
      break;
    case 'b':
      doBounds = true;
      break;
    case 'C':
      collision_constraint = true;
      break;
    case 'd': 
        filename = std::string( optarg );
        dump_data = true;
        break;
    default:
      std::cout << "opt: " << opt << "\n";
      usage(1);
      break;
    }

  }

  if (argc < 2) {
    usage(1);
  }

  Map2D map;

  map.load(argv[argc-1]);

  std::vector<unsigned char> buf;

  map.rasterize(Map2D::RASTER_DISTANCE, buf, 0);
  savePNG_RGB24("dist.png", map.grid.nx(), map.grid.ny(), 
                map.grid.nx()*4, &buf[0], true);

  map.rasterize(Map2D::RASTER_COST, buf, 0);
  savePNG_RGB24("cost.png", map.grid.nx(), map.grid.ny(), 
                map.grid.nx()*4, &buf[0], true);

  map.rasterize(Map2D::RASTER_OCCUPANCY, buf, 0);
  savePNG_RGB24("occupancy.png", map.grid.nx(), map.grid.ny(), 
                map.grid.nx()*4, &buf[0], true);

  MatX q0, q1;

  vec2f p0(x0, y0), p1(x1, y1);
  if (p0.x() == p0.y() && p0 == p1) {
    p0 = map.grid.bbox().p0.trunc();
    p1 = map.grid.bbox().p1.trunc();
  }
  
  MotionOptimizer chomper( NULL, errorTol, 0, max_iter );
  generateInitialTraj(chomper, N, p0, p1, q0, q1);
  
  chomper.getTrajectory().setObjectiveType( otype );
  
  MapCollisionFunction map_collision_function(
                             2,
                             3,
                             1,
                             gamma,
                             &map );

  chomper.setCollisionFunction( &map_collision_function);

  DebugObserver dobs;
  chomper.setObserver( &dobs );
 
  chomper.setAlpha( alpha );
  
  chomper.setAlgorithm( alg ); 

  if ( collision_constraint ){
      chomper.doCollisionConstraint(); 
  }
  
  if ( doBounds ){
      MatX upper(1,2), lower(1,2);
      upper << 3,3;
      lower << -3,-3;
      chomper.setBounds( lower, upper );
  }
  
  if (do_covariant ){ chomper.doCovariantOptimization(); }


#ifdef MZ_HAVE_CAIRO
  PdfEmitter* pe = NULL;

  if ( doPDF >= -1 )
  {
    char buf[1024];
    sprintf(buf, "%s_g%f_a%f_o%s_%s_%s_.pdf",
            algorithmToString( alg ).c_str(),
            gamma, alpha,
            otype == MINIMIZE_VELOCITY ? "vel" : "accel",
            do_covariant ? "covariant" : "non-covariant",
            collision_constraint ? "constr_coll" : "obj_coll" );

    pe = new PdfEmitter(map,
                        chomper.getTrajectory().getXi(),
                        doPDF,
                        buf,
                        dump_data);
    chomper.setObserver( pe );
  }

#endif
  
  chomper.dontSubsample();

  chomper.solve();


#ifdef MZ_HAVE_CAIRO
  if ( pe ) {
      pe->appendInfoToFile( filename );
      delete pe;
  }
#endif

  return 0;

}
