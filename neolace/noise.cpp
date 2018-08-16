int noise2(int x, int y)
{
	int tmp = noise_hash[(y + NOISE_SEED) % 256];
	return noise_hash[(tmp + x) % 256];
}

float linear_interp(float x, float y, float s)
{
	return x + s * (y - x);
}

float smooth_interp(float x, float y, float s)
{
	return linear_interp(x, y, s * s * (3 - 2 * s));
}

float noise2d(float x, float y)
{
	int xi = x;
	int yi = y;
	float xf = x - xi;
	float yf = y - yi;
	int s = noise2(xi, yi);
	int t = noise2(xi + 1, yi);
	int u = noise2(xi, yi + 1);
	int v = noise2(xi + 1, yi + 1);
	float low = smooth_interp(s, t, xf);
	float high = smooth_interp(y, v, xf);
	return smooth_interp(low, high, yf);
}

float perlin2d(float x, float y, float freq, int depth)
{
	float xa = x * freq;
	float ya = y * freq;
	float amp = 1;
	float fin = 0;
	float div = 0;

	int i;
	for (i = 0; i < depth; i++) {
		div += 256 * amp;
		fin += noise2d(xa, ya) * amp;
		amp /= 2;
		xa *= 2;
		ya *= 2;
	}
	return fin / div;
}
