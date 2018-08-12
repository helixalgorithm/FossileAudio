
#include <stdlib.h>
class tools{
public:
	typedef float* cursorType;
	// i rep. 2 power of i, first 1024
	float right(float v1, float k1, float v2, float k2, cursorType &cursor, int pos, int i)
	{ 
		float AX = (float)(i / 2);
		float AY = v1 + k1 * AX;
		float BY = v2 - k2 * AX;
		float UY = (AY + BY) / 2;
		if (i > 1) {
			float Uk = (BY - AY) / i;
			i = i / 2;
			cursor[pos - i] = right(v1, k1, AY, Uk, cursor, pos - i, i);// left side is the lower half: 512..256..128..64..32..16..8..4..2..1
			cursor[pos + i] = right(AY, Uk, v2, k2, cursor, pos + i, i);// right side is the higher half: 512 & 512..768 & 256..896 & 128..960 & 64..992 & 32..1008 & 16..1016 & 8..1019 & 4..1022 & 2..1023 & 1
		}
		return UY;
	}
};