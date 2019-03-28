/********************************************************************
 ********************************************************************
 **
 ** libhungarian by Cyrill Stachniss, 2004
 ** http://www2.informatik.uni-freiburg.de/~stachnis/misc.html
 **
 ** Modified and adapted from C to C++ by Justin Buchanan
 **
 ** Solving the Minimum Assignment Problem using the
 ** Hungarian Method.
 **
 ** ** This file may be freely copied and distributed! **
 **
 ** Parts of the used code was originally provided by the
 ** "Stanford GraphGase", but I made changes to this code.
 ** As asked by  the copyright node of the "Stanford GraphGase",
 ** I hereby proclaim that this file are *NOT* part of the
 ** "Stanford GraphGase" distrubition!
 **
 ** This file is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied
 ** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 ** PURPOSE.
 **
 ********************************************************************
 ********************************************************************/

#pragma once

#include <vector>

namespace Hungarian {

typedef enum {
  MODE_MINIMIZE_COST,
  MODE_MAXIMIZE_UTIL,
} MODE;

typedef enum {
  NOT_ASSIGNED,
  ASSIGNED,
} ASSIGN;

using Matrix = std::vector<std::vector<int>>;

struct Result {
  // True if the algorithm completed and found a solution.
  bool success = false;
  // The solution
  Matrix assignment;
  // A normalized form of the input cost matrix.
  Matrix cost;
  // The costs incurred by the assignment
  int totalCost = 0;
  std::vector<int> col_mate, unchosen_row, row_dec, slack_row, row_mate, parent_row, col_inc, slack;
  bool solve(const Matrix &input, MODE mode);
};


const bool verbose = false;

Matrix normalizeInput(const Matrix &input, MODE mode) {
  const int org_rows = input.size(), org_cols = input[0].size();

  // is the number of cols not equal to number of rows?
  // if yes, expand with 0-cols / 0-cols
  const int mrank = std::max(org_rows, org_cols);

  Matrix output;
  output.resize(mrank, std::vector<int>(mrank, 0));

  int max_cost = 0;
  for (int i = 0; i < org_rows; i++) {
    for (int j = 0; j < org_cols; j++) {
      output[i][j] = input[i][j];
      max_cost = std::max(max_cost, output[i][j]);
    }
  }

  if (mode == MODE_MAXIMIZE_UTIL) {
    for (int i = 0; i < org_rows; i++) {
      for (int j = 0; j < org_cols; j++) {
        output[i][j] = max_cost - output[i][j];
      }
    }
  }

  return output;
}

bool Result::solve(const Hungarian::Matrix &input, Hungarian::MODE mode) {
  success = true;
  this->cost = normalizeInput(input, mode);

  const int INF = std::numeric_limits<int>::max();

  // mrank == rows == cols. it's a square matrix.
  const int mrank = cost.size();

  assignment.resize(mrank, std::vector<int>(mrank, NOT_ASSIGNED));

  int j, k, l, s, t, q, unmatched;

  int cost = 0;

  col_mate = std::vector<int>(mrank, 0);
  unchosen_row = std::vector<int>(mrank, 0);
  row_dec = std::vector<int>(mrank, 0);
  slack_row = std::vector<int>(mrank, 0);
  row_mate = std::vector<int>(mrank, 0);
  parent_row = std::vector<int>(mrank, 0);
  col_inc = std::vector<int>(mrank, 0);
  slack = std::vector<int>(mrank, 0);

  for (int i = 0; i < mrank; ++i) {
    for (int j = 0; j < mrank; ++j) {
      assignment[i][j] = NOT_ASSIGNED;
    }
  }

  // Begin subtract column minima in order to start with lots of zeroes 12
  if (verbose) fprintf(stderr, "Using heuristic\n");
  for (l = 0; l < mrank; l++) {
    s = this->cost[0][l];
    for (k = 1; k < mrank; k++) {
      if (this->cost[k][l] < s) {
        s = this->cost[k][l];
      }
    }
    cost += s;
    if (s != 0) {
      for (k = 0; k < mrank; k++) {
        this->cost[k][l] -= s;
      }
    }
  }
  // End subtract column minima in order to start with lots of zeroes 12

  // Begin initial state 16
  t = 0;
  for (l = 0; l < mrank; l++) {
    row_mate[l] = -1;
    parent_row[l] = -1;
    col_inc[l] = 0;
    slack[l] = INF;
  }
  for (k = 0; k < mrank; k++) {
    s = this->cost[k][0];
    for (l = 1; l < mrank; l++) {
      if (this->cost[k][l] < s) {
        s = this->cost[k][l];
      }
    }
    row_dec[k] = s;
    for (l = 0; l < mrank; l++) {
      if (s == this->cost[k][l] && row_mate[l] < 0) {
        col_mate[k] = l;
        row_mate[l] = k;
        if (verbose) fprintf(stderr, "matching col %d==row %d\n", l, k);
        goto row_done;
      }
    }
    col_mate[k] = -1;
    if (verbose) fprintf(stderr, "node %d: unmatched row %d\n", t, k);
    unchosen_row[t++] = k;
  row_done:;
  }
  // End initial state 16

  // Begin Hungarian algorithm 18
  if (t == 0) goto done;
  unmatched = t;
  while (true) {
    if (verbose) fprintf(stderr, "Matched %d rows.\n", mrank - t);
    q = 0;
    while (true) {
      while (q < t) {
        // Begin explore node q of the forest 19
        {
          k = unchosen_row[q];
          s = row_dec[k];
          for (l = 0; l < mrank; l++)
            if (slack[l]) {
              int del;
              del = this->cost[k][l] - s + col_inc[l];
              if (del < slack[l]) {
                if (del == 0) {
                  if (row_mate[l] < 0) goto breakthru;
                  slack[l] = 0;
                  parent_row[l] = k;
                  if (verbose)
                    fprintf(stderr, "node %d: row %d==col %d--row %d\n", t,
                      row_mate[l], l, k);
                  unchosen_row[t++] = row_mate[l];
                }
                else {
                  slack[l] = del;
                  slack_row[l] = k;
                }
              }
            }
        }
        // End explore node q of the forest 19
        q++;
      }

      // Begin introduce a new zero into the matrix 21
      s = INF;
      for (l = 0; l < mrank; l++)
        if (slack[l] && slack[l] < s) s = slack[l];
      for (q = 0; q < t; q++) {
        row_dec[unchosen_row[q]] += s;
      }
      for (l = 0; l < mrank; l++)
        if (slack[l]) {
          slack[l] -= s;
          if (slack[l] == 0) {
            // Begin look at a new zero 22
            k = slack_row[l];
            if (verbose)
              fprintf(stderr,
                "Decreasing uncovered elements by %d produces zero at "
                "[%d,%d]\n",
                s, k, l);
            if (row_mate[l] < 0) {
              for (int j = l + 1; j < mrank; j++) {
                if (slack[j] == 0) {
                  col_inc[j] += s;
                }
              }
              goto breakthru;
            }
            else {
              parent_row[l] = k;
              if (verbose)
                fprintf(stderr, "node %d: row %d==col %d--row %d\n", t,
                  row_mate[l], l, k);
              unchosen_row[t++] = row_mate[l];
            }
            // End look at a new zero 22
          }
        }
        else {
          col_inc[l] += s;
        }
      // End introduce a new zero into the matrix 21
    }
  breakthru:
    // Begin update the matching 20
    if (verbose) fprintf(stderr, "Breakthrough at node %d of %d!\n", q, t);
    while (true) {
      int j = col_mate[k];
      col_mate[k] = l;
      row_mate[l] = k;
      if (verbose) fprintf(stderr, "rematching col %d==row %d\n", l, k);
      if (j < 0) break;
      k = parent_row[j];
      l = j;
    }
    // End update the matching 20
    if (--unmatched == 0) goto done;
    // Begin get ready for another stage 17
    t = 0;
    for (l = 0; l < mrank; l++) {
      parent_row[l] = -1;
      slack[l] = INF;
    }
    for (k = 0; k < mrank; k++)
      if (col_mate[k] < 0) {
        if (verbose) fprintf(stderr, "node %d: unmatched row %d\n", t, k);
        unchosen_row[t++] = k;
      }
    // End get ready for another stage 17
  }
done:

  // Begin doublecheck the solution 23
  for (k = 0; k < mrank; k++) {
    for (l = 0; l < mrank; l++) {
      if (this->cost[k][l] < row_dec[k] - col_inc[l]) return false;
    }
  }
  for (k = 0; k < mrank; k++) {
    l = col_mate[k];
    if (l < 0 || this->cost[k][l] != row_dec[k] - col_inc[l]) return false;
  }
  k = 0;
  for (l = 0; l < mrank; l++) {
    if (col_inc[l]) {
      k++;
    }
  }
  if (k > mrank) return false;
  // End doublecheck the solution 23
  // End Hungarian algorithm 18

  for (int i = 0; i < mrank; ++i) {
    assignment[i][col_mate[i]] = ASSIGNED;
    /*TRACE("%d - %d\n", i, col_mate[i]);*/
  }
  for (k = 0; k < mrank; ++k) {
    for (l = 0; l < mrank; ++l) {
      /*TRACE("%d ",result.this->cost[k][l]-row_dec[k]+col_inc[l]);*/
      this->cost[k][l] = this->cost[k][l] - row_dec[k] + col_inc[l];
    }
    /*TRACE("\n");*/
  }
  for (int i = 0; i < mrank; i++) {
    cost += row_dec[i];
  }
  for (int i = 0; i < mrank; i++) {
    cost -= col_inc[i];
  }
  if (verbose) fprintf(stderr, "Cost is %d\n", cost);

  totalCost = cost;
  return true;
}

void PrintMatrix(const Hungarian::Matrix &m) {
  const int rows = m.size(), cols = m[0].size();
  fprintf(stderr, "\n");
  for (int i = 0; i < rows; i++) {
    fprintf(stderr, " [");
    for (int j = 0; j < cols; j++) {
      fprintf(stderr, "%5d ", m[i][j]);
    }
    fprintf(stderr, "]\n");
  }
  fprintf(stderr, "\n");
}

}  // namespace Hungarian
