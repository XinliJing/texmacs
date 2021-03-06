
/******************************************************************************
 * MODULE     : QTMApplication.hpp
 * DESCRIPTION:
 * COPYRIGHT  :
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMAPPLICATION_HPP
#define QTMAPPLICATION_HPP

#include <QApplication>
#include "string.hpp"

/*
 FIXME: We would like to do the following
 
 #ifdef USE_EXCEPTIONS
 class QTMApplication... blah blah
 
 #else
 
 typedef QApplication QTMApplication;
 
 #endif
 
 But MOC has trouble with conditional compilation.
 */

/*! QTMApplication
 
 Reimplements notify() in order to catch exceptions thrown from event handlers
 and slots.
 
 NOTE: see http://qt-project.org/forums/viewthread/17731 for the reason why
 the constructor takes an int&
 */
class QTMApplication: public QApplication {
  Q_OBJECT
  
public:
  QTMApplication (int& argc, char** argv) :
    QApplication (argc, argv) { }

  virtual bool notify (QObject* receiver, QEvent* event)
  {
    try {
      return QApplication::notify (receiver, event);
    }
    catch (string s) {
        //c_string cs (s);
        //tm_failure (cs);
        //qt_error << "Thrown " << s << LF;
      the_exception= s;
    }
    return false;
  }
};

#endif   // QTMAPPLICATION_HPP
