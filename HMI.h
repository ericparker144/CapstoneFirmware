#define SHADOW 3
#define RADIUS 5

enum pages {MAIN, SETTINGS, TEMP_ADJUST};
enum settings {WIFI};

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t colour)
{
        GD.Begin(RECTS);
        GD.ColorRGB(colour);
        GD.Vertex2f(x, y);
        GD.Vertex2f(x+w, y+h);
}

void drawRoundedRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t colour)
{
        GD.LineWidth(RADIUS*16);
        drawRect(x+RADIUS, y+RADIUS, w-2*RADIUS, h-2*RADIUS, colour);
        GD.LineWidth(1*16);
}





struct button {
    char tag;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t font;
    uint32_t colour;
    char txt[20];

    void draw(char _tag)
    {
        GD.Tag(tag);
        if(_tag == tag) {
            drawRoundedRect(x, y, w, h, 0xF1C40F);
        }
        else {
            drawRoundedRect(x, y, w, h, colour);
        }
        GD.ColorRGB(0xffffff);
        GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);
        GD.Tag(255);
    }
};


struct keyboard_key {
  char tag;
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
  int16_t font;
  uint32_t colour;
  char txt[2];

  void draw(char _tag)
  {
    GD.Tag(tag);
    if(_tag == tag) {
        drawRoundedRect(x, y, w, h, 0xF1C40F);
    }
    else {
        drawRoundedRect(x, y, w, h, colour);
    }
    GD.ColorRGB(0xffffff);
    GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);

  }
};


class keyboard {
  public:
    int16_t keyboard_start_x;
    int16_t keyboard_start_y;
    int16_t key_spacing;
    int16_t keydim;
    uint32_t key_colour;
    int16_t key_font;
    boolean shift = false;

    keyboard_key keyboard_1;
    keyboard_key keyboard_2;
    keyboard_key keyboard_3;
    keyboard_key keyboard_4;
    keyboard_key keyboard_5;
    keyboard_key keyboard_6;
    keyboard_key keyboard_7;
    keyboard_key keyboard_8;
    keyboard_key keyboard_9;
    keyboard_key keyboard_0;

    keyboard_key keyboard_Q;
    keyboard_key keyboard_W;
    keyboard_key keyboard_E;
    keyboard_key keyboard_R;
    keyboard_key keyboard_T;
    keyboard_key keyboard_Y;
    keyboard_key keyboard_U;
    keyboard_key keyboard_I;
    keyboard_key keyboard_O;
    keyboard_key keyboard_P;

    keyboard_key keyboard_A;
    keyboard_key keyboard_S;
    keyboard_key keyboard_D;
    keyboard_key keyboard_F;
    keyboard_key keyboard_G;
    keyboard_key keyboard_H;
    keyboard_key keyboard_J;
    keyboard_key keyboard_K;
    keyboard_key keyboard_L;

    button keyboard_shift_lock;
    keyboard_key keyboard_Z;
    keyboard_key keyboard_X;
    keyboard_key keyboard_C;
    keyboard_key keyboard_V;
    keyboard_key keyboard_B;
    keyboard_key keyboard_N;
    keyboard_key keyboard_M;
    button keyboard_BS;

    keyboard_key keyboard_sh1;
    keyboard_key keyboard_sh2;
    keyboard_key keyboard_sh3;
    keyboard_key keyboard_sh4;
    keyboard_key keyboard_sh5;
    keyboard_key keyboard_sh6;
    keyboard_key keyboard_sh7;
    keyboard_key keyboard_sh8;
    keyboard_key keyboard_sh9;
    keyboard_key keyboard_sh0;

    keyboard_key keyboard_q;
    keyboard_key keyboard_w;
    keyboard_key keyboard_e;
    keyboard_key keyboard_r;
    keyboard_key keyboard_t;
    keyboard_key keyboard_y;
    keyboard_key keyboard_u;
    keyboard_key keyboard_i;
    keyboard_key keyboard_o;
    keyboard_key keyboard_p;

    keyboard_key keyboard_a;
    keyboard_key keyboard_s;
    keyboard_key keyboard_d;
    keyboard_key keyboard_f;
    keyboard_key keyboard_g;
    keyboard_key keyboard_h;
    keyboard_key keyboard_j;
    keyboard_key keyboard_k;
    keyboard_key keyboard_l;

    keyboard_key keyboard_z;
    keyboard_key keyboard_x;
    keyboard_key keyboard_c;
    keyboard_key keyboard_v;
    keyboard_key keyboard_b;
    keyboard_key keyboard_n;
    keyboard_key keyboard_m;

    keyboard(int16_t x_start, int16_t y_start, int16_t key_space, int16_t key_dimension, uint32_t key_col, int16_t key_fnt) {

      keyboard_start_x = x_start;
      keyboard_start_y = y_start;
      key_spacing = key_space;
      keydim = key_dimension;
      key_colour = key_col;
      key_font = key_fnt;

      keyboard_1 = keyboard_key{'1',keyboard_start_x,keyboard_start_y,keydim,keydim,key_font,key_colour,"1"};
      keyboard_2 = keyboard_key{'2',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"2"};
      keyboard_3 = keyboard_key{'3',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"3"};
      keyboard_4 = keyboard_key{'4',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"4"};
      keyboard_5 = keyboard_key{'5',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"5"};
      keyboard_6 = keyboard_key{'6',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"6"};
      keyboard_7 = keyboard_key{'7',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"7"};
      keyboard_8 = keyboard_key{'8',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"8"};
      keyboard_9 = keyboard_key{'9',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"9"};
      keyboard_0 = keyboard_key{'0',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"0"};

      keyboard_Q = keyboard_key{'Q',keyboard_start_x,keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"Q"};
      keyboard_W = keyboard_key{'W',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"W"};
      keyboard_E = keyboard_key{'E',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"E"};
      keyboard_R = keyboard_key{'R',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"R"};
      keyboard_T = keyboard_key{'T',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"T"};
      keyboard_Y = keyboard_key{'Y',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"Y"};
      keyboard_U = keyboard_key{'U',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"U"};
      keyboard_I = keyboard_key{'I',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"I"};
      keyboard_O = keyboard_key{'O',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"O"};
      keyboard_P = keyboard_key{'P',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"P"};

      keyboard_A = keyboard_key{'A',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"A"};
      keyboard_S = keyboard_key{'S',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"S"};
      keyboard_D = keyboard_key{'D',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"D"};
      keyboard_F = keyboard_key{'F',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"F"};
      keyboard_G = keyboard_key{'G',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"G"};
      keyboard_H = keyboard_key{'H',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"H"};
      keyboard_J = keyboard_key{'J',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"J"};
      keyboard_K = keyboard_key{'K',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"K"};
      keyboard_L = keyboard_key{'L',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"L"};

      keyboard_shift_lock = button{15,keyboard_start_x,keyboard_start_y+3*(keydim+key_spacing),2*keydim+key_spacing,keydim,key_font,key_colour,"Sh"};
      keyboard_Z = keyboard_key{'Z',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"Z"};
      keyboard_X = keyboard_key{'X',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"X"};
      keyboard_C = keyboard_key{'C',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"C"};
      keyboard_V = keyboard_key{'V',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"V"};
      keyboard_B = keyboard_key{'B',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"B"};
      keyboard_N = keyboard_key{'N',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"N"};
      keyboard_M = keyboard_key{'M',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"M"};
      keyboard_BS = button{16,keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"<-"};

      //If shift is pressed:

      keyboard_sh1 = keyboard_key{'-',keyboard_start_x,keyboard_start_y,keydim,keydim,key_font,key_colour,"-"};
      keyboard_sh2 = keyboard_key{'.',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"."};
      keyboard_sh3 = keyboard_key{',',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,","};
      keyboard_sh4 = keyboard_key{'(',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"("};
      keyboard_sh5 = keyboard_key{')',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,")"};
      keyboard_sh6 = keyboard_key{'[',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"["};
      keyboard_sh7 = keyboard_key{']',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"]"};
      keyboard_sh8 = keyboard_key{'{',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"{"};
      keyboard_sh9 = keyboard_key{'}',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"}"};
      keyboard_sh0 = keyboard_key{'_',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y,keydim,keydim,key_font,key_colour,"_"};

      keyboard_q = keyboard_key{'q',keyboard_start_x,keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"q"};
      keyboard_w = keyboard_key{'w',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"w"};
      keyboard_e = keyboard_key{'e',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"e"};
      keyboard_r = keyboard_key{'r',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"r"};
      keyboard_t = keyboard_key{'t',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"t"};
      keyboard_y = keyboard_key{'y',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"y"};
      keyboard_u = keyboard_key{'u',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"u"};
      keyboard_i = keyboard_key{'i',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"i"};
      keyboard_o = keyboard_key{'o',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"o"};
      keyboard_p = keyboard_key{'p',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y+(keydim+key_spacing),keydim,keydim,key_font,key_colour,"p"};

      keyboard_a = keyboard_key{'a',keyboard_start_x+1*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"a"};
      keyboard_s = keyboard_key{'s',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"s"};
      keyboard_d = keyboard_key{'d',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"d"};
      keyboard_f = keyboard_key{'f',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"f"};
      keyboard_g = keyboard_key{'g',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"g"};
      keyboard_h = keyboard_key{'h',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"h"};
      keyboard_j = keyboard_key{'j',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"j"};
      keyboard_k = keyboard_key{'k',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"k"};
      keyboard_l = keyboard_key{'l',keyboard_start_x+9*(keydim+key_spacing),keyboard_start_y+2*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"l"};

      keyboard_z = keyboard_key{'z',keyboard_start_x+2*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"z"};
      keyboard_x = keyboard_key{'x',keyboard_start_x+3*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"x"};
      keyboard_c = keyboard_key{'c',keyboard_start_x+4*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"c"};
      keyboard_v = keyboard_key{'v',keyboard_start_x+5*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"v"};
      keyboard_b = keyboard_key{'b',keyboard_start_x+6*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"b"};
      keyboard_n = keyboard_key{'n',keyboard_start_x+7*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"n"};
      keyboard_m = keyboard_key{'m',keyboard_start_x+8*(keydim+key_spacing),keyboard_start_y+3*(keydim+key_spacing),keydim,keydim,key_font,key_colour,"m"};
    }

    void draw(char _tag) {
      if (shift) {
        keyboard_0.draw(_tag);
        keyboard_1.draw(_tag);
        keyboard_2.draw(_tag);
        keyboard_3.draw(_tag);
        keyboard_4.draw(_tag);
        keyboard_5.draw(_tag);
        keyboard_6.draw(_tag);
        keyboard_7.draw(_tag);
        keyboard_8.draw(_tag);
        keyboard_9.draw(_tag);

        keyboard_Q.draw(_tag);
        keyboard_W.draw(_tag);
        keyboard_E.draw(_tag);
        keyboard_R.draw(_tag);
        keyboard_T.draw(_tag);
        keyboard_Y.draw(_tag);
        keyboard_U.draw(_tag);
        keyboard_I.draw(_tag);
        keyboard_O.draw(_tag);
        keyboard_P.draw(_tag);

        keyboard_A.draw(_tag);
        keyboard_S.draw(_tag);
        keyboard_D.draw(_tag);
        keyboard_F.draw(_tag);
        keyboard_G.draw(_tag);
        keyboard_H.draw(_tag);
        keyboard_J.draw(_tag);
        keyboard_K.draw(_tag);
        keyboard_L.draw(_tag);

        keyboard_Z.draw(_tag);
        keyboard_X.draw(_tag);
        keyboard_C.draw(_tag);
        keyboard_V.draw(_tag);
        keyboard_B.draw(_tag);
        keyboard_N.draw(_tag);
        keyboard_M.draw(_tag);
      }
      else {
        keyboard_sh0.draw(_tag);
        keyboard_sh1.draw(_tag);
        keyboard_sh2.draw(_tag);
        keyboard_sh3.draw(_tag);
        keyboard_sh4.draw(_tag);
        keyboard_sh5.draw(_tag);
        keyboard_sh6.draw(_tag);
        keyboard_sh7.draw(_tag);
        keyboard_sh8.draw(_tag);
        keyboard_sh9.draw(_tag);

        keyboard_q.draw(_tag);
        keyboard_w.draw(_tag);
        keyboard_e.draw(_tag);
        keyboard_r.draw(_tag);
        keyboard_t.draw(_tag);
        keyboard_y.draw(_tag);
        keyboard_u.draw(_tag);
        keyboard_i.draw(_tag);
        keyboard_o.draw(_tag);
        keyboard_p.draw(_tag);

        keyboard_a.draw(_tag);
        keyboard_s.draw(_tag);
        keyboard_d.draw(_tag);
        keyboard_f.draw(_tag);
        keyboard_g.draw(_tag);
        keyboard_h.draw(_tag);
        keyboard_j.draw(_tag);
        keyboard_k.draw(_tag);
        keyboard_l.draw(_tag);

        keyboard_z.draw(_tag);
        keyboard_x.draw(_tag);
        keyboard_c.draw(_tag);
        keyboard_v.draw(_tag);
        keyboard_b.draw(_tag);
        keyboard_n.draw(_tag);
        keyboard_m.draw(_tag);
      }

      keyboard_shift_lock.draw(_tag);
      keyboard_BS.draw(_tag);
    }

    void shift_press() {
      shift = !shift;
    }
};
