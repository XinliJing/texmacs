
/******************************************************************************
* MODULE     : tracked_fromtex.cpp
* DESCRIPTION: Conversion from LaTeX to TeXmacs with source tracking
* COPYRIGHT  : (C) 2014 Joris van der Hoeven
*******************************************************************************
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
******************************************************************************/

#include "Tex/convert_tex.hpp"
#include "analyze.hpp"
#include "hashset.hpp"
#include "scheme.hpp"

/******************************************************************************
* Add markers to LaTeX document
******************************************************************************/

bool
skip_curly (string s, int& i) {
  int n= N(s);
  skip_spaces (s, i);
  if (i >= n || s[i] != '{') return false;
  i++;
  while (true) {
    if (i >= n) return false;
    if (s[i] == '{') {
      if (!skip_curly (s, i)) return false;
      continue;
    }
    if (s[i] == '}') {
      i++;
      return true;
    }
    i++;
  }
}

static bool
skip_square (string s, int& i) {
  int n= N(s);
  skip_spaces (s, i);
  if (i >= n || s[i] != '[') return false;
  i++;
  while (true) {
    if (i >= n) return false;
    if (s[i] == '[') {
      if (!skip_square (s, i)) return false;
      continue;
    }
    if (s[i] == ']') {
      i++;
      return true;
    }
    i++;
  }
}

static bool
parse_begin_end (string s, int& i) {
  int n= N(s);
  if (i == n || s[i] != '\\') return false;
  if (test (s, i, "\\begin") || test (s, i, "\\end")) {
    int b= i;
    if (s[b+1] == 'b') i += 6;
    else i += 4;
    skip_spaces (s, i);
    int c= i;
    if (skip_curly (s, i)) {
      string cmd  = "begin-" * s (c+1, i-1);
      string type = latex_type (cmd);
      int    arity= latex_arity (cmd);
      if (type != "undefined") {
        if (s[b+1] == 'e') return true;
        bool opt= (arity < 0);
        if (opt) arity= -1 - arity;
        for (int j=0; j<arity; j++)
          if (!skip_curly (s, i)) { i=b; return false; }
        if (opt) {
          int j= i;
          skip_spaces (s, j);
          if (j<n && s[j] == '[') {
            i=j;
            if (!skip_square (s, i)) { i=b; return false; }
          }
        }
        return true;
      }
    }
    i= b;
  }
  return false;
}

static void
mark_begin (string& r, int i, hashset<int>& l) {
  if (l->contains (i)) return;
  l->insert (i);
  r << "{\\blx{" << as_string (i) << "}}";
}

static void
mark_end (string& r, int i, hashset<int>& l) {
  if (l->contains (i)) return;
  l->insert (i);
  r << "{\\elx{" << as_string (i) << "}}";
}

bool
skip_latex_spaces (string s, int& i) {
  int ln= 0, n= N(s);
  while (i<n) {
    skip_spaces (s, i);
    if (i<n && s[i] == '%') {
      skip_line (s, i);
      if (i<n && s[i] == '\n') i++;
    }
    else if (i<n && s[i] == '\n') {
      i++;
      ln++;
    }
    else break;
  }
  return ln >= 2;
}

string
latex_mark (string s, hashset<int>& l) {
  // FIXME: attention to Windows line breaks
  string r;
  int i= 0, n= N(s);
  skip_latex_spaces (s, i);
  mark_begin (r, i, l);
  while (true) {
    int b= i;
    if (skip_latex_spaces (s, i)) {
      if (i < n) {
        mark_end (r, b, l);
        r << s (b, i);
        mark_begin (r, i, l);
        continue;
      }
    }
    if (i >= n) {
      mark_end (r, b, l);
      r << s (b, i);
      break;
    }
    if (parse_begin_end (s, i)) {
      if (s[b+1] == 'b') {
        r << s (b, i);
        b= i;
        if (skip_latex_spaces (s, i)) i= b;
        r << s (b, i);
        mark_begin (r, i, l);
      }
      else {
        mark_end (r, b, l);
        r << s (b, i);
      }
    }
    else {
      i++;
      r << s (b, i);
    }
  }
  return r;
}

/******************************************************************************
* Grouping markers in TeXmacs document
******************************************************************************/

static bool
contains_itm (tree t) {
  if (is_atomic (t)) return false;
  else if (is_compound (t, "itm", 1)) return true;
  else {
    for (int i=0; i<N(t); i++)
      if (contains_itm (t[i])) return true;
    return false;
  }
}

static tree
marked_group (string b, string e, tree body) {
  if (contains_itm (body)) return body;
  return compound ("mlx", b * ":" * e, body);
}

tree
texmacs_group_markers (tree t) {
  if (is_atomic (t)) return t;
  else {
    int i, n= N(t);
    tree r (t, n);
    for (i=0; i<n; i++)
      r[i]= texmacs_group_markers (t[i]);
    if (is_concat (r) && N(r) >=2 &&
        is_compound (r[0], "blx", 1) &&
        is_compound (r[N(r)-1], "elx", 1))
      {
        string b= as_string (r[0][0]);
        string e= as_string (r[N(r)-1][0]);
        r= r (1, N(r)-1);
        if (N(r) == 0) r= "";
        else if (N(r) == 1) r= r[0];
        return marked_group (b, e, r);
      }
    if (is_document (r)) {
      tree d (DOCUMENT);
      for (int i=0; i<n; i++) {
        if (is_compound (r[i], "blx", 1) ||
            (is_concat (r[i]) && N(r[i]) > 1 &&
             is_compound (r[i][0], "blx", 1) &&
             !is_compound (r[i][N(r[i])-1], "elx", 1))) {
          int j= i+1;
          bool ok= false;
          while (j<n) {
            if (is_compound (r[j], "blx", 1)) break;
            if (is_compound (r[j], "elx", 1)) { ok= true; break; }
            if (is_concat (r[j])) {
              bool br= false;
              for (int k=0; k<N(r[j]); j++)
                if (is_compound (r[j][k], "blx", 1)) {
                  br= true; break; }
                else if (is_compound (r[j][k], "elx", 1)) {
                  br= true; ok= (k == N(r[j]) - 1); break; }
              if (br) break;
            }
            j++;
          }
          if (ok) {
            tree body (DOCUMENT);
            tree bt= r[i];
            tree et= r[j];
            if (is_concat (bt)) {
              tree rem= bt (1, N(bt));
              if (N(rem) == 1) rem= rem[0];
              body << rem;
              bt= bt[0];
            }
            body << A (r (i+1, j));
            if (is_concat (et)) {
              tree rem= et (0, N(et)-1);
              if (N(rem) == 1) rem= rem[0];
              body << rem;
              et= et[N(et)-1];
            }
            string b= as_string (bt[0]);
            string e= as_string (et[0]);
            i= j;
            if (N(body) == 0) continue;
            if (N(body) == 1) body= body[0];
            d << marked_group (b, e, body);
            continue;
          }
        }
        d << r[i];
      }
      r= d;
    }
    if (is_document (r) && N(r) >= 2 &&
        is_compound (r[0], "blx", 1) &&
        is_compound (r[N(r)-1], "elx", 1))
      {
        string b= as_string (r[0][0]);
        string e= as_string (r[N(r)-1][0]);
        r= r (1, N(r)-1);
        marked_group (b, e, r);
      }
    return r;
  }
}

tree
texmacs_correct_markers (tree t, int b, int e) {
  if (is_atomic (t)) return t;
  else if (is_compound (t, "mlx", 2)) {
    array<string> a= tokenize (as_string (t[0]), ":");
    if (N(a) != 2)
      return texmacs_correct_markers (t[1], b, e);
    int sb= as_int (a[0]);
    int se= as_int (a[1]);
    if (sb <= b || se >= e)
      return texmacs_correct_markers (t[1], b, e);
    return compound ("mlx", t[0], texmacs_correct_markers (t[1], sb, se));
  }
  else {
    int i, n= N(t);
    tree r (t, n);
    for (i=0; i<n; i++)
      r[i]= texmacs_correct_markers (t[i], b, e);
    return r;
  }
}

/******************************************************************************
* Remove markers from TeXmacs document
******************************************************************************/

static tree
texmacs_unmark (tree t, bool all) {
  if (is_atomic (t)) return t;
  else if (is_compound (t, "blx", 1)) return "";
  else if (is_compound (t, "elx", 1)) return "";
  else if (all && is_compound (t, "mlx", 2)) return texmacs_unmark (t[1], all);
  else if (is_document (t)) {
    int i, n= N(t);
    tree r (DOCUMENT);
    for (i=0; i<n; i++)
      if (is_compound (t[i], "blx", 1));
      else if (is_compound (t[i], "elx", 1));
      else r << texmacs_unmark (t[i], all);
    return simplify_document (r);
  }
  else {
    int i, n= N(t);
    tree r (t, n);
    for (i=0; i<n; i++)
      r[i]= texmacs_unmark (t[i], all);
    if (is_concat (r)) return simplify_concat (r);
    return r;
  }
}

tree texmacs_unmark (tree t) { return texmacs_unmark (t, true); }
tree texmacs_clean_markers (tree t) { return texmacs_unmark (t, false); }

/******************************************************************************
* LaTeX -> TeXmacs conversion with source tracking
******************************************************************************/

tree
tracked_latex_to_texmacs (string s, bool as_pic) {
  if (get_preference ("latex->texmacs:source-tracking", "off") != "on")
    return latex_document_to_tree (s, as_pic);
  hashset<int> l;
  string ms= latex_mark (s, l);
  cout << HRULE << "Marked latex" << LF << HRULE << ms << LF;
  tree mt= latex_document_to_tree (ms, as_pic);
  tree body= extract (mt, "body");
  cout << HRULE << "Marked texmacs" << LF << HRULE << body << LF;
  body= texmacs_group_markers (body);
  cout << HRULE << "Grouped texmacs" << LF << HRULE << body << LF;
  body= texmacs_correct_markers (body, -1000000000, 1000000000);
  cout << HRULE << "Corrected texmacs" << LF << HRULE << body << LF;
  body= texmacs_clean_markers (body);
  cout << HRULE << "Cleaned texmacs" << LF << HRULE << body << LF;
  tree lsrc (ASSOCIATE, "latex-source", s);
  tree ltar (ASSOCIATE, "latex-target", change_doc_attr (mt, "body", body));
  tree atts (COLLECTION, lsrc, ltar);
  body= texmacs_unmark (body);
  mt= change_doc_attr (mt, "body", body);
  mt= change_doc_attr (mt, "attachments", atts);
  return mt;
}