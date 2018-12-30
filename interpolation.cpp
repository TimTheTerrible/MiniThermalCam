#include <Arduino.h>

float get_point(float *p, uint8_t rows, uint8_t cols, int8_t row, int8_t col);
void set_point(float *p, uint8_t rows, uint8_t cols, int8_t row, int8_t col, float f);
void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t row, int8_t col);
void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t row, int8_t col);
float cubicInterpolate(float p[], float col);
float bicubicInterpolate(float p[], float row, float col);
void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, 
                       float *dest, uint8_t dest_rows, uint8_t dest_cols);

float get_point(float *p, uint8_t rows, uint8_t cols, int8_t row, int8_t col) {
  if (col < 0)
    col = 0;

  if (row < 0)
    row = 0;

  if (col >= cols)
    col = cols - 1;

  if (row >= rows)
    row = rows - 1;
  
  return p[row * cols + col];
}

void set_point(float *p, uint8_t rows, uint8_t cols, int8_t row, int8_t col, float f) {
  
  if ((col < 0) || (col >= cols))
    return;

  if ((row < 0) || (row >= rows))
    return;

  int pixels = rows * cols;

  p[row * cols + (cols - col)] = f;
}

void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, 
                       float *dest, uint8_t dest_rows, uint8_t dest_cols) {
  
  float mu_col = (src_cols - 1.0) / (dest_cols - 1.0);
  float mu_row = (src_rows - 1.0) / (dest_rows - 1.0);

  float adj_2d[16]; // matrix for storing adjacents
  
  for (uint8_t row_idx=0; row_idx < dest_rows; row_idx++) {

    for (uint8_t col_idx=0; col_idx < dest_cols; col_idx++) {

       float col = col_idx * mu_col;
       float row = row_idx * mu_row;
       //Serial.print("("); Serial.print(row_idx); Serial.print(", "); Serial.print(col_idx); Serial.print(") = ");
       //Serial.print("("); Serial.print(row); Serial.print(", "); Serial.print(col); Serial.print(") = ");
       get_adjacents_2d(src, adj_2d, src_rows, src_cols, col, row);
       /*
       Serial.print("[");
       for (uint8_t i=0; i<16; i++) {
         Serial.print(adj_2d[i]); Serial.print(", ");
       }
       Serial.println("]");
       */
       float frac_col = col - (int)col; // we only need the ~delta~ between the points
       float frac_row = row - (int)row; // we only need the ~delta~ between the points
       float out = bicubicInterpolate(adj_2d, frac_col, frac_row);
       //Serial.print("\tInterp: "); Serial.println(out);
       set_point(dest, dest_rows, dest_cols, col_idx, row_idx, out);
    }
  }
}

// p is a list of 4 points, 2 to the left, 2 to the right
float cubicInterpolate(float p[], float col) {
    float r = p[1] + (0.5 * col * (p[2] - p[0] + col*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + col*(3.0*(p[1] - p[2]) + p[3] - p[0]))));
  /*
    Serial.print("interpolating: ["); 
    Serial.print(p[0],2); Serial.print(", ");
    Serial.print(p[1],2); Serial.print(", ");
    Serial.print(p[2],2); Serial.print(", ");
    Serial.print(p[3],2); Serial.print("] w/"); Serial.print(col); Serial.print(" = ");
    Serial.println(r);
  */
    return r;
}

// p is a 16-point 4x4 array of the 2 rows & columns left/right/above/below
float bicubicInterpolate(float p[], float row, float col) {
    float arr[4] = {0,0,0,0};
    arr[0] = cubicInterpolate(p+0, col);
    arr[1] = cubicInterpolate(p+4, col);
    arr[2] = cubicInterpolate(p+8, col);
    arr[3] = cubicInterpolate(p+12, col);
    return cubicInterpolate(arr, row);
}

// src is rows*cols and dest is a 4-point array passed in already allocated!
void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t row, int8_t col) {
    //Serial.print("("); Serial.print(col); Serial.print(", "); Serial.print(row); Serial.println(")");
    // pick two items to the left
    dest[0] = get_point(src, rows, cols, col-1, row);
    dest[1] = get_point(src, rows, cols, col, row);
    // pick two items to the right
    dest[2] = get_point(src, rows, cols, col+1, row);
    dest[3] = get_point(src, rows, cols, col+2, row);
}


// src is rows*cols and dest is a 16-point array passed in already allocated!
void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t row, int8_t col) {
    //Serial.print("("); Serial.print(col); Serial.print(", "); Serial.print(row); Serial.println(")");
    float arr[4];
    for (int8_t delta_row = -1; delta_row < 3; delta_row++) { // -1, 0, 1, 2
        float *aRow = dest + 4 * (delta_row+1); // index into each chunk of 4
        for (int8_t delta_col = -1; delta_col < 3; delta_col++) { // -1, 0, 1, 2
            aRow[delta_col+1] = get_point(src, rows, cols, col+delta_col, row+delta_row);
        }
    }
}
