
/******************************************************************************
* MODULE     : qt_utilities.mm
* DESCRIPTION: Utilities for QT
* COPYRIGHT  : (C) 2007  Massimiliano Gubinelli
*******************************************************************************
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
******************************************************************************/

#include "qt_utilities.hpp"
#include <QImage>
#include <QPrinter>
#include <QPainter>
#include <QCoreApplication>
#include "dictionary.hpp"
#include "converter.hpp"

#ifdef USE_GS
#include "Ghostscript/gs_utilities.hpp"
#endif

QRect
to_qrect (const coord4 & p) {
  float c= 1.0/PIXEL;
  return QRect (p.x1*c, -p.x4*c, (p.x3-p.x1+PIXEL-1)*c, (p.x4-p.x2+PIXEL-1)*c);
}

QPoint
to_qpoint (const coord2 & p) {
  float c= 1.0/PIXEL;
  return QPoint (p.x1*c, -p.x2*c);
}

QSize
to_qsize (const coord2 & p) {
  float c= 1.0/PIXEL;
  return QSize (p.x1*c, p.x2*c);
}

coord4
from_qrect (const QRect & rect) {
  SI c1, c2, c3, c4;
  c1= rect.x() * PIXEL;
  c2= -(rect.y() + rect.height()) * PIXEL;       
  c3= (rect.x() + rect.width()) * PIXEL;
  c4= -rect.y() * PIXEL;
  return coord4 (c1, c2, c3, c4);
}

coord2
from_qpoint (const QPoint & pt) {
  SI c1, c2;
  c1= pt.x() * PIXEL;
  c2= -pt.y() * PIXEL;
  return coord2 (c1, c2);
}

coord2
from_qsize (const QSize & s) {
  SI c1, c2;
  c1= s.width() * PIXEL;
  c2= s.height() * PIXEL;
  return coord2 (c1, c2);
}

QString
to_qstring (string s) {
  char* p= as_charp (s);
  QString nss (p);
  tm_delete_array (p);
  return nss;
}

string
from_qstring (const QString &s) {
  QByteArray arr= s.toUtf8 ();
  const char* cstr= arr.constData ();
  return utf8_to_cork (string ((char*) cstr));
}

QString
to_qstring_utf8 (string s) {
  s= cork_to_utf8 (s);
  char* p= as_charp (s);
  QString nss= QString::fromUtf8 (p, N(s));
  tm_delete_array (p);
  return nss;
}

string
qt_translate (string s) {
  string out_lan= get_output_language ();
  return tm_var_encode (translate (s, "english", out_lan));
}

bool
qt_supports_image (url u) {
  string s= suffix (u);
  if (s == "ps" || s == "eps" || s == "pdf") return false;
  return true;
}

void
qt_image_size (url image, int& w, int& h) {
  QImage im= QImage (to_qstring (concretize (image)));
  if (im.isNull ()) {
    cerr << "Cannot read image file '" << image << "'" << LF;
    w= 35; h= 35;
  }
  else {
    w= im.width ();
    h= im.height ();
  }
}

void
qt_convert_image (url image, url dest, int w, int h) {
  QImage im (to_qstring (concretize (image)));
  if (im.isNull ()) {
    cerr << "Cannot read image file '" << image << "'" << LF;
  }
  else {
    if (w > 0 && h > 0) im= im.scaled (w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    im.scaled (w, h).save (to_qstring (concretize (dest)));
  }
}

void
qt_image_to_eps (url image, url eps, int w_pt, int h_pt, int dpi) {
  static const char* d= "0123456789ABCDEF";
  QImage im (to_qstring (concretize (image)));
  if (im.isNull ()) {
    cerr << "Cannot read image file '" << image << "'" << LF;
  }
  else {
    if (dpi > 0 && w_pt > 0 && h_pt > 0) {
      int ww= w_pt * dpi / 72;
      int hh= h_pt * dpi / 72;
      if (ww < im.width () || hh < im.height ()) {
        im= im.scaled (ww, hh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      }
    }
    string res;
    string sw= as_string (im.width ());
    string sh= as_string (im.height ());
    res << "%!PS-Adobe-3.0 EPSF-3.0\n%%Creator: TeXmacs\n%%BoundingBox: ";
    res << "0 0 " << sw << " " << sh << "\n";
    res << "%%LanguageLevel: 2\n%%Pages: 1\n%%DocumentData: Clean7Bit\n";
    res <<  sw << " " << sh << " scale\n";
    res <<  sw << " " << sh << " 8 [" << sw << " 0 0 -" << sh << " 0 " << sh << "]\n";
    res << "{currentfile 3 " << sw << " mul string readhexstring pop} bind\nfalse 3 colorimage\n";
    int v, i= 0, j= 0, l= 0;
    for (j= 0; j < im.height (); j++) {
      for (i=0; i < im.width (); i++) {
        l++;
        QRgb p= im.pixel (i, j);
        v= qRed (p);
        res << d [(v >> 4)] << d [v % 16];
        v= qGreen (p);
        res << d [(v >> 4)] << d [v % 16];
        v= qBlue (p);
        res << d [(v >> 4)] << d [v % 16];
        if (l > 10) {
          res << "\n";
          l= 0;
        }
      }
    }
    res << "\n%%EOF";
    save_string (eps, res);
#ifdef USE_GS
    url temp= url_temp (".eps");
    gs_to_eps (eps, temp);
    copy (temp, eps);
    remove (temp);
#endif
  }
}

string qt_application_directory ()
{
  return  string (QCoreApplication::applicationDirPath () .toAscii() .constData());
//  return from_qstring (QCoreApplication::applicationDirPath ());
}

