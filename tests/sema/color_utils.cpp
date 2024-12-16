#include "tests/sema/common.hpp"

static const char* Program = R"END(
ulam 1;

quark ColorUtils {
  typedef Unsigned(8) Channel;
  typedef Bits(8) Byte;
  typedef Channel ARGB[4];

  ARGB color(Int hex) {
    return color((Bits) hex);
  }

  ARGB color(Unsigned hex) {
    return color((Bits) hex);
  }

  ARGB color(Bits hex) {
    ARGB ret;
    ret[0] = (Channel) ((hex >> 24) & 0xff);
    ret[1] = (Channel) ((hex >> 16) & 0xff);
    ret[2] = (Channel) ((hex >>  8) & 0xff);
    ret[3] = (Channel) ((hex >>  0) & 0xff);
    return ret;
  }

  ARGB color(Byte red, Byte green, Byte blue) {
    ARGB ret;
    ret[0] = (Channel) 0xff;
    ret[1] = (Channel) (red & 0xff);
    ret[2] = (Channel) (green & 0xff);
    ret[3] = (Channel) (blue & 0xff);
    return ret;
  }

  ARGB modify(ARGB this, Unsigned percent) {
    return between(color(255,255,255),this,percent);
  }

  ARGB between(ARGB this, ARGB that, Unsigned percentOfThis) {
    if (percentOfThis > 100u) percentOfThis = 100u;
    Unsigned percentOfThat = (Unsigned) 100u - percentOfThis;
    for (Int i = 1; i < 4; ++i) {
      Unsigned new = (this[i] * percentOfThis +
                      that[i] * percentOfThat) / 100u;
      this[i] = (Channel) new;
    }
    return this;
  }

  ARGB brighter(ARGB than) {
    than[0] = 0xff;
    if (than[1] == 0u && than[2] == 0u && than[3] == 0u)
      return color((Unsigned) 0x030303);
    for (Int i = 1; i < 4; ++i) {
      if (than[i] < 3u) than[i] = 3u;
      than[i] = (Channel) (100u*than[i]/70u);

    }
    return than;
  }

  ARGB dimmer(ARGB than) {
    than[0] = 0xff;
    for (Int i = 1; i < 4; ++i)
      than[i] = (Channel) (70u*than[i]/100u);
    return than;
  }
}
)END";

int main() {
    analyze_and_print(Program, "ColorUtils");
}
