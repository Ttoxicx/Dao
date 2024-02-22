#pragma once

#define DAO_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define DAO_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define DAO_PIN(a, min_value, max_value) DAO_MIN(max_value, DAO_MAX(a, min_value))

#define DAO_VALID_INDEX(idx, range) (((idx) >= 0) && ((idx) < (range)))
#define DAO_PIN_INDEX(idx, range) DAO_PIN(idx, 0, (range)-1)

#define DAO_SIGN(x) ((((x) > 0.0f) ? 1.0f : 0.0f) + (((x) < 0.0f) ? -1.0f : 0.0f))
