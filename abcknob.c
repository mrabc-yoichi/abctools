/* based from g_vslider.c from pd 0.43.4 */
/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* abcknob.c*/
/* Release 1a 20140417 */
/* Release 1b-47 20170811 */
/* Copyright (c) 2014-2017 Yoichi HIRATA */
/* http://tkrkapps.blogspot.jp */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"

#include "g_all_guis.h"
#include <math.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

#define KNOBDEFSIZE 30
#define METERBACKSIZE 5

/* ------------ [abcknob] ----------------------- */

t_widgetbehavior abcknob_widgetbehavior;
static t_class *abcknob_class;

/* widget helper functions */

typedef struct _abcknob
{
    t_iemgui    x_gui;
    int         x_pos;
    int         x_val;
    int         x_lin0_log1;
    int         x_steady;
    double       x_min;
    double       x_max;
    double       x_base; //gauge base value (differs to init value)
    double       x_base_k;
    double   x_k;
    t_float x_fval;

	int		Origin_x;
	int		Origin_y;
	float	Deg_dest;
	float	Deg_base;
	int		x_degoffset;
	int xknobdeg_x;
	int xknobdeg_y;
    int metertogl;
} t_abcknob;


static void abcknob_setdest(t_abcknob *x){
	x->Origin_x = (int)(x->x_gui.x_w/2);
	x->Origin_y = (int)(x->x_gui.x_h/2);

    x->Deg_dest = (240.0-300.0*((x->x_val/100.0 )/(127 + x->x_degoffset)) );
	x->Deg_base = (240.0-300.0*((x->x_base_k)/(127 + x->x_degoffset)) ) ;
}

void abcknob_set_x_k(t_abcknob *x){

    double vrange = x->x_max - x->x_min;
    if(x->x_lin0_log1 == 0){
        if( (int)(vrange)%2 != 0) x->x_degoffset = 1;
        else x->x_degoffset = 0;
        x->x_k = vrange / 127;
        x->x_base_k = (x->x_base - x->x_min) / x->x_k;
    }
    else{
        x->x_k = log(x->x_max/x->x_min)/127;
        x->x_base_k = log(x->x_base/x->x_min)/x->x_k;
    }
   }

double abcknob_calc_out(t_abcknob *x){
  double out;
    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (double)((x->x_val * 0.01) * x->x_k + x->x_min);
    if((out < 1.0e-5)&&(out > -1.0e-5))
        out = 0.0;
    return out;

}

static void abcknob_draw_update(t_gobj *client, t_glist *glist)
{
    t_abcknob *x = (t_abcknob *)client;
    if (glist_isvisible(glist))
	{
		t_canvas *canvas=glist_getcanvas(glist);


        abcknob_setdest(x);
		int xpos=text_xpix(&x->x_gui.x_obj, glist);
		int ypos=text_ypix(&x->x_gui.x_obj, glist);

        //ABCMEMO: 90度刻みの方向(真上と真横)は arc -style pieslice が滲んで真っ直ぐの線にならないので
        //決め打ちでarcは消してlindでインディケータの線をひく
		int xknobsel = 0;
		switch((int)x->Deg_dest){
		  case 0:
			xknobsel = 1;
			x->xknobdeg_x = 1;
			x->xknobdeg_y = 0;
			break;
		  case 90:
			xknobsel = 1;
			x->xknobdeg_x = 0;
			x->xknobdeg_y = -1;
			break;
		  case 180:
			xknobsel = 1;
			x->xknobdeg_x = -1;
			x->xknobdeg_y = 0;
			break;
		}

		if(xknobsel){
		sys_vgui(".x%lx.c itemconfigure %lxKNOB1 -start %d -extent %d -state hidden\n",
						canvas, x, (int)x->Deg_dest , 0);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB2 -state normal -fill #%6.6x\n",
						canvas, x,  x->x_gui.x_fcol);
		sys_vgui(".x%lx.c coords %lxKNOB2 %d %d %d %d\n",
						canvas, x, xpos + x->Origin_x, ypos + x->Origin_y,
						xpos + x->Origin_x + x->Origin_x * x->xknobdeg_x, ypos + x->Origin_y + x->Origin_y * x->xknobdeg_y) ;
		}

		else{
		sys_vgui(".x%lx.c itemconfigure %lxKNOB1 -start %d -extent %d -outline #%6.6x -state normal\n",
				canvas, x, (int)x->Deg_dest , 0, x->x_gui.x_fcol);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB2 -state hidden\n",
				canvas, x);
		}

    char basebgvisible[20];
    if(x->x_min == x->x_max) {
      sprintf(basebgvisible,"hidden");
    }
    else{
      sprintf(basebgvisible,"normal");
    }

		//gauge
		sys_vgui(".x%lx.c itemconfigure %lxBASE3 -start %d -extent %d -outline #%6.6x -state %s\n",
					canvas, x, (int)x->Deg_base, (int)x->Deg_dest - (int)x->Deg_base , x->x_gui.x_fcol,
          basebgvisible);
		//kuro
		if(x->Deg_dest > x->Deg_base){
            sys_vgui(".x%lx.c itemconfigure %lxBASE1 -start %d -extent %d -outline #%6.6x -state normal\n",
					canvas, x, (int)x->Deg_dest, 241 - (int)x->Deg_dest, x->x_gui.x_bcol);
            sys_vgui(".x%lx.c itemconfigure %lxBASE2 -start %d -extent %d -outline #%6.6x -state normal\n",
					canvas, x, -60 + 3*(x->x_degoffset), (int)x->Deg_base + 60 - 3*(x->x_degoffset), x->x_gui.x_bcol);
        }
        else{
            sys_vgui(".x%lx.c itemconfigure %lxBASE1 -start %d -extent %d -outline #%6.6x -state normal\n",
					canvas, x, (int)x->Deg_base, 241 - (int)x->Deg_base, x->x_gui.x_bcol);
            sys_vgui(".x%lx.c itemconfigure %lxBASE2 -start %d -extent %d -outline #%6.6x -state normal\n",
					canvas, x, -60 + 3*(x->x_degoffset), (int)x->Deg_dest + 60 - 3*(x->x_degoffset), x->x_gui.x_bcol);
        }
        if((int)x->Deg_dest == 240  ||  (int)x->Deg_base == 240){
            sys_vgui(".x%lx.c itemconfigure %lxBASE1 -state hidden\n",
            canvas, x);
        }
        if((int)x->Deg_dest == -60  ||  (int)x->Deg_base == -60){
            sys_vgui(".x%lx.c itemconfigure %lxBASE2 -state hidden\n",
            canvas, x);
        }

        //ABCMEMO: This IF statement should be OK to set " =< 1 ",
        //but only in Windows environment, drawings are not correct when the knob is around leftmost
        //due to some problems of implementation of Tcl/Tk.
        //So instead, " < 4 " is set as an interim solution for such Windows' jerk behavior.
        if(abs((int)x->Deg_dest - (int)x->Deg_base) < 4 ){
            sys_vgui(".x%lx.c itemconfigure %lxBASE3 -state hidden\n",
            canvas, x);

        }


/*      //for debugging:
        char aex[20];
        sprintf(aex,"DegBase %d",(int)x->Deg_base);
        post(aex);
        sprintf(aex,"Degdest %d",(int)x->Deg_dest);
        post(aex);
*/


    char axx[20];
    sprintf(axx,"hidden");
    if(x->metertogl){   //メーターの文字列操作
    //ABCMEMO: TODO: Make resizing font automatically
        double out = abcknob_calc_out(x);

        if (fabs(out)<1000)
        {
            int outi = ((int)(out * 100)) % (100000);

            if(abs(outi)<100000){

                if(abs(outi)>=10000) {
                    sprintf(axx,"%d",(int)(outi/100));
                    //if(outi%100 != 0) strcat(axx,".");
                }
                else if(abs(outi)>=1000) {
                    float outif = (float)( (int)(outi/10) / 10.0);
                    if(outi%100 != 0) {
                        sprintf(axx,"%2.1f",outif);
                        //if(outi%10 != 0) strcat(axx,".");
                    }
                    else{
                        sprintf(axx,"%2.0f",outif);
                    }
                }
                else if(abs(outi)>=100){
                    if(outi%100 != 0){
                        sprintf(axx,"%1.2f",(float)(outi/100.0));
                    }
                    else sprintf(axx,"%1.0f",(float)(outi/100.0));
                }
                else{
                    if(outi==0) sprintf(axx,"0");
                    else sprintf(axx,"%.2f",(float)(outi/100.0));

                }
            }
        }
        else{
            if(out>0) sprintf(axx,"+++");
            else sprintf(axx,"---");
        }

        sys_vgui(".x%lx.c itemconfigure %lxVAL -text {%s}\n",
                canvas, x, axx);

        sprintf(axx,"normal");
    }

    sys_vgui(".x%lx.c itemconfigure %lxVAL -state %s \n",
            canvas, x, axx);
    sys_vgui(".x%lx.c itemconfigure %lxMETB -state %s \n",
            canvas, x, axx);
	}



}

static void abcknob_draw_new(t_abcknob *x, t_glist *glist)
{

	if( (int)(x->x_max - x->x_min)%2 != 0) x->x_degoffset = 1;
	else x->x_degoffset = 0;

    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    int zoomlabel =
        1 + (IEMGUI_ZOOM(x)-1) * (x->x_gui.x_ldx >= 0 && x->x_gui.x_ldy >= 0);

    t_canvas *canvas=glist_getcanvas(glist);

    //abcknob_setdest(x);

    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxOUT%d outlet]\n",
             canvas,
             xpos, ypos + x->x_gui.x_h+1-2*IEMGUI_ZOOM(x),
             xpos+IOWIDTH, ypos + x->x_gui.x_h,
             x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxIN%d inlet]\n",
            canvas,
            xpos, ypos,
            xpos+IOWIDTH, ypos-1+2*IEMGUI_ZOOM(x),
            x, 0);

	/*
  Drawing Objects:
  BASE1: gauge background from the leftmost to right
  BASE2: gauge background from the rightmost to left
  BASE3: gauge frontportion from the base value to the destination value
  KNOB1: knob radial line from center to the destination value
  KNOB2: knob exist horizontal or perpendicular line activated only when the destination is 0, 90, 180 or 270 degrees
  METB: white background of the numerical value when it is activated
  */
	sys_vgui(".x%lx.c create arc %d %d %d %d -start %d -extent %d -style arc -width %d -outline #%6.6x -tags %lxBASE3\n",
			canvas, xpos, ypos, xpos + x->x_gui.x_h, ypos + x->x_gui.x_h ,
            (int)x->Deg_base, (int)x->Deg_dest - (int)x->Deg_base ,
            1 + 2 * IEMGUI_ZOOM(x), x->x_gui.x_fcol, x);

	//kuro
    sys_vgui(".x%lx.c create arc %d %d %d %d -start %d -extent %d -style arc -width %d -outline #%6.6x -tags %lxBASE1\n",
			canvas, xpos, ypos, xpos + x->x_gui.x_h, ypos + x->x_gui.x_h ,
            241, (int)x->Deg_dest - 241,
            1 + 2 * IEMGUI_ZOOM(x), x->x_gui.x_bcol, x);
    sys_vgui(".x%lx.c create arc %d %d %d %d -start %d -extent %d -style arc -width %d -outline #%6.6x -tags %lxBASE2\n",
			canvas, xpos, ypos, xpos + x->x_gui.x_h, ypos + x->x_gui.x_h ,
            -60, (int)x->Deg_base + 60,
            1 + 2 * IEMGUI_ZOOM(x), x->x_gui.x_bcol, x);
    sys_vgui(".x%lx.c create line %d %d %d %d -width %d -fill #%6.6x -state hidden -tags %lxKNOB2\n",
            canvas, xpos+x->Origin_x, ypos+x->Origin_y, xpos+x->Origin_x, ypos,
            1 + 2 * IEMGUI_ZOOM(x), x->x_gui.x_fcol, x);

    sys_vgui(".x%lx.c create arc %d %d %d %d -start %d -extent -%d -style pieslice -width %d -outline #%6.6x -tags %lxKNOB1\n",
            canvas, xpos, ypos, xpos + x->x_gui.x_h, ypos + x->x_gui.x_h,
            (int)x->Deg_dest, 0,
            1 + 2 * IEMGUI_ZOOM(x), x->x_gui.x_fcol, x);

   sys_vgui(".x%lx.c create oval %d %d %d %d -width 1 -fill white -outline white -tags %lxMETB\n",
            canvas, xpos + x->x_gui.x_h/METERBACKSIZE, ypos + x->x_gui.x_h/METERBACKSIZE,
            xpos + x->x_gui.x_h*(METERBACKSIZE-1)/METERBACKSIZE,
            ypos + x->x_gui.x_h*(METERBACKSIZE-1)/METERBACKSIZE,
            x);

    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
             -font {{%s} -%d %s} -fill #%6.6x -tags [list %lxLABEL label text]\n",
             canvas, xpos+x->x_gui.x_ldx* zoomlabel,
             ypos+x->x_gui.x_ldy * zoomlabel,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
             x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x), sys_fontweight,
             x->x_gui.x_lcol, x);


   char axx[20];
    double out = abcknob_calc_out(x);
    sprintf(axx,"%4g",out);
   sys_vgui(".x%lx.c create text %d %d -width %d -text {%s} -anchor center\
             -font {{%s} -%d %s} -fill #%6.6x -tags %lxVAL\n",
             canvas, xpos+x->x_gui.x_h/2, ypos+x->x_gui.x_h*6/11, 100,
             axx ,x->x_gui.x_font , x->x_gui.x_h/3, sys_fontweight,
            x->x_gui.x_lcol, x);
    if(x->metertogl){sprintf(axx,"normal"); }
    else{sprintf(axx,"hidden");}
    sys_vgui(".x%lx.c itemconfigure %lxVAL -state %s \n",
            canvas, x, axx);
    sys_vgui(".x%lx.c itemconfigure %lxMETB -state %s \n",
            canvas, x, axx);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}


static void abcknob_draw_move(t_abcknob *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    int zoomlabel =
        1 + (IEMGUI_ZOOM(x)-1) * (x->x_gui.x_ldx >= 0 && x->x_gui.x_ldy >= 0);

    t_canvas *canvas=glist_getcanvas(glist);


    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
             canvas, x, xpos+x->x_gui.x_ldx * zoomlabel,
             ypos+x->x_gui.x_ldy * zoomlabel);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c coords %lxOUT%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos + x->x_gui.x_h+1-2*IEMGUI_ZOOM(x),
             xpos+IOWIDTH, ypos + x->x_gui.x_h);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c coords %lxIN%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos,
				xpos+IOWIDTH, ypos-1+2*IEMGUI_ZOOM(x));

	//my add
	abcknob_setdest(x);
    sys_vgui(".x%lx.c coords %lxBASE1 %d %d %d %d\n",
				canvas, x, xpos, ypos,
				xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxBASE2 %d %d %d %d\n",
				canvas, x, xpos, ypos,
				xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
	sys_vgui(".x%lx.c coords %lxBASE3 %d %d %d %d\n",
				canvas, x, xpos, ypos,
				xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
	sys_vgui(".x%lx.c coords %lxKNOB1 %d %d %d %d\n",
				canvas, x, xpos, ypos,
				xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
	sys_vgui(".x%lx.c coords %lxKNOB2 %d %d %d %d\n",
				canvas, x, xpos + x->Origin_x, ypos + x->Origin_y,
				xpos + x->Origin_x + x->Origin_x * x->xknobdeg_x, ypos + x->Origin_y + x->Origin_y * x->xknobdeg_y);

    sys_vgui(".x%lx.c coords %lxVAL %d %d\n",
				canvas, x, xpos+x->x_gui.x_h/2, ypos+x->x_gui.x_h*6/11) ;
    sys_vgui(".x%lx.c coords %lxMETB %d %d %d %d\n",
				canvas, x,  xpos + x->x_gui.x_h/METERBACKSIZE, ypos + x->x_gui.x_h/METERBACKSIZE,
            xpos + x->x_gui.x_h*(METERBACKSIZE-1)/METERBACKSIZE,
            ypos + x->x_gui.x_h*(METERBACKSIZE-1)/METERBACKSIZE);

}

static void abcknob_draw_erase(t_abcknob* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

	sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
   	sys_vgui(".x%lx.c delete %lxBASE1\n", canvas, x);
	sys_vgui(".x%lx.c delete %lxBASE2\n", canvas, x);
	sys_vgui(".x%lx.c delete %lxBASE3\n", canvas, x);
	sys_vgui(".x%lx.c delete %lxKNOB1\n", canvas, x);
	sys_vgui(".x%lx.c delete %lxKNOB2\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxMETERBACK\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxVAL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void abcknob_draw_config(t_abcknob* x,t_glist* glist)
{
	t_canvas *canvas=glist_getcanvas(glist);


    abcknob_set_x_k(x);

    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x), sys_fontweight,
             x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");

    sys_vgui(".x%lx.c itemconfigure %lxVAL -font {{%s} -%d %s} \n",
             canvas, x,  x->x_gui.x_font , x->x_gui.x_h/3, sys_fontweight);


    /*   sys_vgui(".x%lx.c itemconfigure %lxKNOB1 -outline #%6.6x\n", canvas,
			x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxKNOB2 -fill #%6.6x\n", canvas,
             x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE2 -outline #%6.6x\n", canvas,
				x, x->x_gui.x_bcol);
	sys_vgui(".x%lx.c itemconfigure %lxBASE3 -outline #%6.6x\n", canvas,
             x, x->x_gui.x_fcol);
*/
   (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void abcknob_draw_io(t_abcknob* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas,
             xpos, ypos + x->x_gui.x_h-1,
             xpos+IOWIDTH, ypos + x->x_gui.x_h,
             x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas,
             xpos, ypos,
             xpos+IOWIDTH, ypos+1,
             x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void abcknob_draw_select(t_abcknob *x, t_glist *glist)
{
//ABCMEMO: 選択時のみ数値を見えるようにする予定
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
	{
		sys_vgui(".x%lx.c itemconfigure %lxBASE1 -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
    sys_vgui(".x%lx.c itemconfigure %lxBASE2 -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB1 -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB2 -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
		sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);

    }
    else
	{
    sys_vgui(".x%lx.c itemconfigure %lxBASE1 -outline #%6.6x\n", canvas, x, x->x_gui.x_bcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE2 -outline #%6.6x\n", canvas, x, x->x_gui.x_bcol);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB1 -outline #%6.6x\n", canvas, x, x->x_gui.x_fcol);
		sys_vgui(".x%lx.c itemconfigure %lxKNOB2 -fill #%6.6x\n", canvas, x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
    }
}

void abcknob_draw(t_abcknob *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, abcknob_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        abcknob_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        abcknob_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        abcknob_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        abcknob_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        abcknob_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        abcknob_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vsl widgetbehaviour----------------------------- */


static void abcknob_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_abcknob* x = (t_abcknob*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) - 2;
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h + 5;
}

static void abcknob_save(t_gobj *z, t_binbuf *b)
{
	//ABCMEMO: 保存されるときのオブジェクト名はここで設定している

	t_abcknob *x = (t_abcknob *)z;

	//ABCMEMO Fixed 20170717
	t_symbol *bflcol[3];

    t_symbol *srl[3];

	iemgui_save(&x->x_gui, srl, bflcol);
	//ABCMEMO: Fixed 201707
	binbuf_addv(b, "ssiisfiffiisssiiiisssii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("abcknob"), x->x_base, x->x_gui.x_h,
                (t_float)x->x_min, (t_float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->metertogl);
    binbuf_addv(b, ";");
}

void abcknob_check_height(t_abcknob *x, int h)
{
    if(h < IEM_SL_MINSIZE)
        h = IEM_SL_MINSIZE;
	x->x_gui.x_h = h;
	x->x_gui.x_w = h;
}

void abcknob_check_minmax(t_abcknob *x, double min, double max, double base)
{
    if(x->x_lin0_log1)
    {
        if((min == 0.0)&&(max == 0.0))
            max = 1.0;
        if(max > 0.0)
        {
            if(min <= 0.0)
                min = 0.01*max;
        }
        else
        {
            if(min > 0.0)
                max = 0.01*min;
        }
    }
    x->x_min = min;
	  x->x_max = max;
    x->x_base = base;

    if(x->x_min > x->x_max){
		if(x->x_min < base) x->x_base = x->x_min;
		if(x->x_max > base) x->x_base = x->x_max;
	}
    else{
		if(x->x_min > x->x_base) x->x_base = x->x_min;
		if(x->x_max < x->x_base) x->x_base = x->x_max;


	}

   abcknob_set_x_k(x);



}

static void abcknob_properties(t_gobj *z, t_glist *owner)
{
    t_abcknob *x = (t_abcknob *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);

//ABCMEMO: Fixed 201707
    sprintf(buf, "pdtk_iemgui_dialog %%s |abcknob| \
            --------dimensions(pix)(pix):-------- %g %g base: %d %d size: \
            -----------output-range:----------- %g bottom: %g top: %d \
            %d lin log %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            #%06x #%06x #%06x\n",
            x->x_base, -1e+38 , x->x_gui.x_h, IEM_GUI_MINSIZE,
            x->x_min, x->x_max, 0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->metertogl, -1,/*no multi, but iem-characteristic*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
			0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
	gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void abcknob_bang(t_abcknob *x)
{
    double out;

 /*   if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (double)((x->x_val * 0.01) * x->x_k + x->x_min);
    if((out < 1.0e-5)&&(out > -1.0e-5))
        out = 0.0;
*/
    if (pd_compatibilitylevel < 46) out = abcknob_calc_out(x);
    else out = x->x_fval;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void abcknob_dialog(t_abcknob *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int liloflag = x->x_lin0_log1;

	//int w = (int)atom_getintarg(0, argc, argv);
	float base = (float)atom_getfloatarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady; // = (int)atom_getintarg(17, argc, argv);
    int mtrtogl = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;

    if(mtrtogl)
        x->metertogl = 1;
    else
        x->metertogl = 0;

    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    //x->x_gui.x_w = iemgui_clip_size(w); //no need

    double out;
    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (float)((x->x_val * 0.01) * x->x_k + x->x_min);
    if((out < 1.0e-5)&&(out > -1.0e-5))
        out = 0.0;

    abcknob_check_height(x, h);
	abcknob_check_minmax(x, min, max, base);


    if(x->x_lin0_log1 == liloflag--){
        if(x->x_lin0_log1) x->x_val = log(out/x->x_min)/x->x_k * 100;
        else x->x_val = (out - x->x_min)/x->x_k * 100;
    }
    else x->x_val = x->x_min*100;


    if(x->x_val > 12700)
    {
        x->x_val = 12700;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
    }

    abcknob_set_x_k(x);

    x->x_pos = x->x_val;

    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text*)x);
}

static void abcknob_motion(t_abcknob *x, t_floatarg dx, t_floatarg dy)
//ABCMEMO: rangeがoddのときとevenの時でposの増え方を変えれば中央値でズレない．
//oddの時は127刻み，evenの時は125刻みにしようか．
//もしくはoddの時はx_valに50足しておけば済む話か? <- こっちを採用
{
    int old = x->x_val;

    if(x->x_gui.x_fsf.x_finemoved)
        x->x_pos -= (int)dy ;
    else
        x->x_pos -= 100*(int)dy;

    x->x_val = x->x_pos + (x->x_degoffset == 0)*50;

    if(x->x_val > 12700)
    {
        x->x_val = 12700;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
    }
    x->x_fval = abcknob_calc_out(x);
    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        abcknob_bang(x);
    }


}

static void abcknob_click(t_abcknob *x, t_floatarg xpos, t_floatarg ypos,
                          t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
//    if(!x->x_steady) //ABCMEMO:三角関数で角度出していれるか？
//    x->x_val = (int)(100.0 * (x->x_gui.x_h + text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) - ypos));
    x->x_fval = abcknob_calc_out(x);
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);

    abcknob_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)abcknob_motion, 0, xpos, ypos);
}

static int abcknob_newclick(t_gobj *z, struct _glist *glist,
                            int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_abcknob* x = (t_abcknob *)z;

    if(doit)
    {
        abcknob_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
                       0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void abcknob_set(t_abcknob *x, t_floatarg f)
{
	//ABCMEMO: Fixed 2010707
	//if(x->x_gui.x_isa.x_reverse)
  x->x_fval = f;
	if(x->x_min > x->x_max)
    {
        if(f > x->x_min)
            f = x->x_min;
        if(f < x->x_max)
            f = x->x_max;
    }
    else
    {
        if(f > x->x_max)
            f = x->x_max;
        if(f < x->x_min)
            f = x->x_min;
    }
    if(x->x_lin0_log1) x->x_val = log(f/x->x_min)/x->x_k * 100;
    else x->x_val = (int)(100.0*((f - x->x_min)/x->x_k ) + 0.49999); //四捨五入だな

    x->x_pos = x->x_val;

	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}


//ABCMEMO: 各種msgに対する挙動をここに記述．
//ここでは関数の定義のみ．msgとの関連付けは _setupの中にあるので
//そちらも設定すること

static void abcknob_float(t_abcknob *x, t_floatarg f)
{
    abcknob_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
        abcknob_bang(x);
}

static void abcknob_size(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        abcknob_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}


static void abcknob_delta(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
//ABCMEMO:delta分だけ動く
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void abcknob_pos(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void abcknob_range(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{
    double out;
    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (double)((x->x_val * 0.01) * x->x_k + x->x_min);
    if((out < 1.0e-7)&&(out > -1.0e-7))
        out = 0.0;


    abcknob_check_minmax(x, (float)atom_getfloatarg(0, ac, av),
                         (float)atom_getfloatarg(1, ac, av), x->x_base);
    x->x_val = (out - x->x_min)/x->x_k * 100;
    x->x_pos = x->x_val;

    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void abcknob_base(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{
    abcknob_check_minmax(x,x->x_min, x->x_max, (float)atom_getfloatarg(0, ac, av));
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}


static void abcknob_color(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void abcknob_send(t_abcknob *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void abcknob_receive(t_abcknob *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void abcknob_label(t_abcknob *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void abcknob_label_pos(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void abcknob_label_font(t_abcknob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void abcknob_log(t_abcknob *x)
{
    x->x_lin0_log1 = 1;
    abcknob_check_minmax(x, x->x_min, x->x_max, x->x_base);
}

static void abcknob_lin(t_abcknob *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min);
}

static void abcknob_init(t_abcknob *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void abcknob_steady(t_abcknob *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void abcknob_meter(t_abcknob *x, t_floatarg f)
{
    if(f==0) x->metertogl = 0;
    else x->metertogl = 1;
     (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

//ABCMEMO: Fixed 201707
//static void abcknob_loadbang(t_abcknob *x)
static void abcknob_loadbang(t_abcknob *x, t_floatarg action)
{
	//ABCMEMO: Fixed 201707
	//if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
	if(action == LB_LOAD && x->x_gui.x_isa.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        abcknob_bang(x);
    }
}

static void *abcknob_new(t_symbol *s, int argc, t_atom *argv)
{
    t_abcknob *x = (t_abcknob *)pd_new(abcknob_class);
	//ABCMEMO: deled 201707
	//int bflcol[]={-1, -1, -1};  //ABCMEMO: COLOR 0x RRRR RR00 GGGG GG00 BBBB BB00 と思ったけど違った(笑)
    int h=KNOBDEFSIZE;
    int lilo=0, f=0, ldx=0, ldy=-9;
    int fs=10, v=0, steady=1;
    int mtrtogl = 0;
	double min=0.0, max=(float)(IEM_SL_DEFAULTSIZE-1);
    double base = 0.0;
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
	iem_inttofstyle(&x->x_gui.x_fsf, 0);

	//ABCMEMO: Added 201707
	x->x_gui.x_bcol = 0x808080;
    x->x_gui.x_fcol = 0x00;
	x->x_gui.x_lcol = 0x00;

	//ABCMEMO: パラメータの初期値はここで設定
    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
        base = atom_getfloatarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);

    //bflcol[0] = (int)atom_getintarg(13, argc, argv);
    //bflcol[1] = (int)atom_getintarg(14, argc, argv);
		//bflcol[2] = (int)atom_getintarg(15, argc, argv);
		//ABCMEMO: Deled Above 3 lines and Added Below 1 line 201707
		//pd047ではiemgui_all_loadcolors. pd043ではiemgui_all_colfromload.
		//しかもbflcolの中身の表現方法が変わっているようだ.
		//下位互換性は絶望的っぽい.
		iemgui_all_loadcolors(&x->x_gui, argv+13, argv+14, argv+15);

        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        mtrtogl = (int)atom_getintarg(17, argc, argv);
        //steady = (int)atom_getintarg(17, argc, argv);
    x->x_gui.x_draw = (t_iemfunptr)abcknob_draw;
    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_isa.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;
    x->x_pos = x->x_val;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady != 0) steady = 1;
    x->x_steady = steady;
    if(mtrtogl != 0) mtrtogl = 1;
    x->metertogl = mtrtogl;

    if(!strcmp(x->x_gui.x_snd->s_name, "empty")) x->x_gui.x_fsf.x_snd_able = 0;
    if(!strcmp(x->x_gui.x_rcv->s_name, "empty")) x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if(x->x_gui.x_fsf.x_rcv_able) pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
   //x->x_gui.x_w = iemgui_clip_size(w);
    abcknob_check_height(x, h);
    abcknob_check_minmax(x, min, max, base);
	//ABCMEMO: deled 201707
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    x->x_fval = abcknob_calc_out(x);

    abcknob_setdest(x);


    return (x);
}

static void abcknob_free(t_abcknob *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void abcknob_setup(void)
{
    abcknob_class = class_new(gensym("abcknob"), (t_newmethod)abcknob_new,
                              (t_method)abcknob_free, sizeof(t_abcknob), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)abcknob_new, gensym("abcknob"), A_GIMME, 0);
    class_addbang(abcknob_class,abcknob_bang);
    class_addfloat(abcknob_class,abcknob_float);
    class_addmethod(abcknob_class, (t_method)abcknob_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_loadbang, gensym("loadbang"), 0);
    class_addmethod(abcknob_class, (t_method)abcknob_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_size, gensym("size"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_range, gensym("range"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_base, gensym("base"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_color, gensym("color"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_log, gensym("log"), 0);
    class_addmethod(abcknob_class, (t_method)abcknob_lin, gensym("lin"), 0);
    class_addmethod(abcknob_class, (t_method)abcknob_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_steady, gensym("steady"), A_FLOAT, 0);
    class_addmethod(abcknob_class, (t_method)iemgui_zoom, gensym("zoom"),
        A_CANT, 0);
    class_addmethod(abcknob_class, (t_method)abcknob_meter, gensym("meter"), A_FLOAT, 0);
    abcknob_widgetbehavior.w_getrectfn =    abcknob_getrect;
    abcknob_widgetbehavior.w_displacefn =   iemgui_displace;
    abcknob_widgetbehavior.w_selectfn =     iemgui_select;
    abcknob_widgetbehavior.w_activatefn =   NULL;
    abcknob_widgetbehavior.w_deletefn =     iemgui_delete;
    abcknob_widgetbehavior.w_visfn =        iemgui_vis;
	abcknob_widgetbehavior.w_clickfn =      abcknob_newclick;
	class_setwidget(abcknob_class, &abcknob_widgetbehavior);
    class_sethelpsymbol(abcknob_class, gensym("abcknob"));
    class_setsavefn(abcknob_class, abcknob_save);
    class_setpropertiesfn(abcknob_class, abcknob_properties);
}
