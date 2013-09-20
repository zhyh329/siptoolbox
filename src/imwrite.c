/*
 * -------------------------------------------------------------------------
 * SIP - Scilab Image Processing toolbox
 * Copyright (C) 2002-2009  Ricardo Fabbri
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------------
 */ 

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include "sip_common.h"

static int check_args(char *fname, int opt_pos);

/*-----------------------------------------------------------------
 * int_imwrite: 
 *     interface for imwrite function.
 *     TODO
 *       - Im (HyperMat) doesn't seem to be freed.
 *
 * $Revision: 1.2 $ $Date: 2009-03-29 21:34:48 $
 *-----------------------------------------------------------------*/
SipExport int 
int_imwrite(char *fname) 
{
   /* Interface variables */ 
   int   m1, n1, l1, 
         minlhs=1, maxlhs=1, minrhs=2, maxrhs=3,
         name_rows, name_columns, name,
         nopt, iopos; 
   static rhs_opts opts[]= { 
         {-1,"depth",-1,0,0,NULL},
         {-1,"quality",-1,0,0,NULL},
		   {-1,NULL,-1,0,0,NULL}
   };
   unsigned nv;
   bool stat;

   unsigned long depth, quality;

   /* Other variables */
   short int argtype;

   /* ImageMagick variables */
   ExceptionInfo  exception;
   Image          *image;
   ImageInfo      *image_info;
   PixelPacket    *pix;

   /* -- Deal with the arguments -- */
   nopt = NumOpt();
   CheckRhs(minrhs,maxrhs + nopt);
   CheckLhs(minlhs,maxlhs);

   if ( getOptionals(pvApiCtx,fname,opts) == 0) return false;
   if ( (argtype = check_args(fname, nopt)) == false) return false;

   /* default values if optional arguments are not given */
   iopos=Rhs ;
   if ( opts[0].iPos == -1 ) {
      depth = QuantumDepth;
   } else {
      double dblDepth = 0;
      getScalarDouble(pvApiCtx, opts[0].piAddr, &dblDepth);
      depth = (unsigned long)dblDepth;
      if ( depth != 8) {
        if  (depth == 16 && QuantumDepth < 16) 
           sip_error("depth cannot be 16 in your current setup")
        else if (depth != 16)
           sip_error("depth must be 8 or 16")
      }
   }

   if ( opts[1].iPos == -1 ) {
      quality= 75; /* Default quality/compression for jpeg/png/miff */
   } else { 
      double dblQuality = 0;
      getScalarDouble(pvApiCtx, opts[1].piAddr, &dblQuality);
      quality = (unsigned long) dblQuality;
      if (dblQuality < 0 || quality > 100) 
         sip_error("quality must be in range 0-100")
   }


   /* -- Pass scilab structures to IM -- */
   
   InitializeMagick(NULL);
   GetExceptionInfo(&exception);
   image_info=CloneImageInfo((ImageInfo *) NULL);

   image_info->colorspace = RGBColorspace; // @@@ maybe to take this off
   image_info->monochrome = 0;
   image_info->dither = 0;  // Imagemagick sets this as true by default.
                            // But this changes binary images too much.
   image_info->depth = depth;
   image_info->quality = quality;
   image=AllocateImage(image_info);

   nv = 1;
   switch (argtype) {
      case ARG_2D:
            GetRhsVar(nv++, "d", &m1, &n1, &l1);
            stat = sci_2D_double_matrix_to_magick(fname, l1, m1, n1, image, &pix);
            if (!stat) 
               return false;
            break;

      case ARG_3D:
            stat = sci_3D_double_hypermat_to_magick(fname,nv++,image,&pix); 
            if (!stat)
               return false;
            break;

      case ARG_INDEX_MAP:
            stat = sci_index_map_to_magick(fname, nv, image, &pix);
            if (!stat) 
               return false;
            nv+=2;
            break;
      default:
            return false;
   }

   GetRhsVar(nv++, "c", &name_rows, &name_columns, &name);  

   /* -- write the image -- */
   (void) strncpy(image->filename,cstk(name),MaxTextExtent);
   if (WriteImage(image_info,image) == 0)
      SIP_MAGICK_ERROR;
   
   DestroyImageInfo(image_info);
   DestroyExceptionInfo(&exception);
   DestroyImage(image);
   DestroyMagick();
   return true;
}


/*-----------------------------------------------------------
 * check_args:                                                
 *    Certify the correctness of the number and type of
 *    arguments. 
 *    The returning code indicates the sematic type of the
 *    argument (non-zero), or invalid argument (zero)
 *-----------------------------------------------------------*/
int
check_args(char *fname, int nopts)
{
   int nargs;

   nargs = Rhs - nopts;
   if (nargs == 2) {
      if (VarType(2) != STRING)
         sip_error("argument 2 must be a string")
      switch (VarType(1)) {
         case USUALMATRIX: return ARG_2D; 
         case HYPERMATRIX: return ARG_3D; 
         default: 
            sip_error("argument 1 must be matrix or hypermatrix")
      }
   } 
   /* nargs == 3 */
   if(VarType(3) != STRING)
      sip_error("argument 3 must be a string")
   if((VarType(1) != USUALMATRIX) || (VarType(2) != USUALMATRIX))
      sip_error("the first two arguments must be arrays")

   return ARG_INDEX_MAP;
}
