/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include <numeric>

template <class InputIterator, class T>
T standard_deviation(InputIterator first, InputIterator last, T average) {
  T var=0;
  /* calculate standard deviation (Standardabweichung :)
   * Varianz: D^2X = 1/(testcount - 1) * sum((x - avg(x))^2)
   * Standardabweichung = sqrt(D^2X)
   */
  int i=0;
  for(InputIterator iter=first; iter!= last; ++iter) {
    var += (*iter - average) * (*iter - average);
    i++;
  }
  var = sqrt(var/(i-1));

  return var;
}

template <class InputIterator, class T>
int count_range(InputIterator first, InputIterator last, T min, T max) {
  int i=0;
  for(InputIterator iter=first; iter!= last; ++iter) {
    if(*iter < min || *iter > max) i++;
  }
  return i;
}

