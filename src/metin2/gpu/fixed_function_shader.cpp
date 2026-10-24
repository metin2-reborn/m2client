#include "fixed_function.h"

// The fixed-function pipeline as one GLSL 3.30 program fed by the FFP uniform block. Clip space is
// OpenGL's: z from -w to w, y flipped when rendering into a texture.
namespace metin2::gpu {

    // clang-format off
    const char *const FIXED_FUNCTION_VERTEX_SHADER = R"GLSL(
#version 330 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aDiffuse;
layout(location = 3) in vec4 aSpecular;
layout(location = 4) in vec4 aTex0;
layout(location = 5) in vec4 aTex1;
layout(location = 6) in vec4 aTex2;

struct Light { vec4 diffuse; vec4 specular; vec4 ambient; vec4 pos; vec4 dir; vec4 att; vec4 spot; };
layout(std140) uniform FFP {
    mat4 uWorld; mat4 uView; mat4 uProj; mat4 uNormal; mat4 uTexM[2];
    vec4 uViewport;
    vec4 uMatDiffuse; vec4 uMatAmbient; vec4 uMatSpecular; vec4 uMatEmissive;
    vec4 uGlobalAmbient;
    ivec4 uLightFlags;
    ivec4 uMatSrc;
    ivec4 uFogI;
    vec4 uFogP;
    vec4 uFogColor;
    vec4 uTFactor;
    vec4 uMisc;
    ivec4 uAlpha;
    ivec4 uStageA[2];
    ivec4 uStageB[2];
    ivec4 uStageC[2];
    vec4 uEffect;
    Light uLights[8];
};

out vec4 vColor;
out vec4 vSpec;
out vec4 vTex0;
out vec4 vTex1;
out float vFog;
out float vDepth;
out vec3 vPosView;
out vec3 vNormView;
out vec3 vPosWorld;
out vec4 vDiffuseIn;
out vec4 vSpecularIn;

vec4 texInput(int idx, int dim) {
    vec4 t = idx == 0 ? aTex0 : (idx == 1 ? aTex1 : aTex2);
    if (dim == 1) return vec4(t.x, 1.0, 0.0, 0.0);
    if (dim == 2) return vec4(t.xy, 1.0, 0.0);
    if (dim == 3) return vec4(t.xyz, 1.0);
    return t;
}

vec4 stageTexcoord(int s, vec3 P, vec3 N) {
    int tci = uStageB[s].w;
    int idx = uStageB[s].z;
    int dim = uStageC[s].z;
    vec4 tc;
    if (tci == 1) tc = vec4(N, 1.0);
    else if (tci == 2) tc = vec4(P, 1.0);
    else if (tci == 3) tc = vec4(reflect(normalize(P), N), 1.0);
    else tc = texInput(idx, dim);
    if (uStageC[s].x != 0) tc = uTexM[s] * tc;
    return tc;
}

void main() {
    bool rhw = uMisc.y != 0.0;
    bool hasD = uMisc.z != 0.0;
    bool hasS = uMisc.w != 0.0;
    vec3 P, N = vec3(0.0, 0.0, 1.0);
    vec4 clip;
    if (rhw) {
        float w = aPos.w > 0.0 ? 1.0 / aPos.w : 1.0;
        float px = aPos.x - float(uAlpha.z) + uViewport.w * 0.5;
        float py = aPos.y - float(uAlpha.w) + uViewport.w * 0.5;
        vec2 ndc = vec2(px / uViewport.x * 2.0 - 1.0, 1.0 - py / uViewport.y * 2.0);
        clip = vec4(ndc * w, aPos.z * w, w);
        P = aPos.xyz;
        vColor = hasD ? aDiffuse : vec4(1.0);
        vSpec = hasS ? aSpecular : vec4(0.0);
        vFog = (uFogI.x != 0 && hasS) ? aSpecular.a : 1.0;
        vDepth = uFogI.z != 0 ? aPos.z : -1.0;
        vPosWorld = aPos.xyz;
    } else {
        vec4 wp = uWorld * vec4(aPos.xyz, 1.0);
        if (uEffect.x == 3.0) {
            // Grass: the tip sways with the wind, and a tuft sinks into the ground towards the end
            // of the grass distance instead of popping out. Diffuse alpha is the tuft height / 128.
            float tip = 1.0 - aTex0.y;
            float tuftHeight = aDiffuse.a * 128.0;
            float dist = length((uView * wp).xyz);
            float keep = 1.0 - smoothstep(0.6 * uEffect.w, uEffect.w, dist);
            wp.z -= tip * tuftHeight * (1.0 - keep);
            float sway = tip * tuftHeight * (0.08 + 0.4 * uEffect.z);
            float t = uEffect.y;
            wp.xy += sway * vec2(sin(t * 1.7 + dot(wp.xy, vec2(0.004, 0.003))), 0.5 * cos(t * 1.3 + wp.x * 0.003));
        }
        vec4 vp = uView * wp;
        P = vp.xyz;
        N = normalize(mat3(uNormal) * aNormal);
        clip = uProj * vp;
        clip.xy += vec2(uViewport.w / uViewport.x, -uViewport.w / uViewport.y) * clip.w;
        // Lighting is evaluated per pixel from the interpolated position and normal.
        vColor = hasD ? aDiffuse : vec4(1.0);
        vSpec = hasS ? aSpecular : vec4(0.0);
        // Fog is evaluated per pixel from this distance; only pre-transformed vertices carry it.
        vDepth = uFogI.w != 0 ? length(P) : abs(P.z);
        vFog = 1.0;
        vPosWorld = wp.xyz;
    }
    vPosView = P;
    vNormView = N;
    vDiffuseIn = hasD ? aDiffuse : vec4(1.0);
    vSpecularIn = hasS ? aSpecular : vec4(0.0);
    clip.z = clip.z * 2.0 - clip.w;
    if (uViewport.z < 0.0) clip.y = -clip.y;
    gl_Position = clip;
    vTex0 = stageTexcoord(0, P, N);
    vTex1 = stageTexcoord(1, P, N);
}
)GLSL";

    const char *const FIXED_FUNCTION_FRAGMENT_SHADER = R"GLSL(
#version 330 core
struct Light { vec4 diffuse; vec4 specular; vec4 ambient; vec4 pos; vec4 dir; vec4 att; vec4 spot; };
layout(std140) uniform FFP {
    mat4 uWorld; mat4 uView; mat4 uProj; mat4 uNormal; mat4 uTexM[2];
    vec4 uViewport;
    vec4 uMatDiffuse; vec4 uMatAmbient; vec4 uMatSpecular; vec4 uMatEmissive;
    vec4 uGlobalAmbient;
    ivec4 uLightFlags;
    ivec4 uMatSrc;
    ivec4 uFogI;
    vec4 uFogP;
    vec4 uFogColor;
    vec4 uTFactor;
    vec4 uMisc;
    ivec4 uAlpha;
    ivec4 uStageA[2];
    ivec4 uStageB[2];
    ivec4 uStageC[2];
    vec4 uEffect;
    Light uLights[8];
};
uniform sampler2D uSampler0;
uniform sampler2D uSampler1;

in vec4 vColor;
in vec4 vSpec;
in vec4 vTex0;
in vec4 vTex1;
in float vFog;
in float vDepth;
in vec3 vPosView;
in vec3 vNormView;
in vec3 vPosWorld;
in vec4 vDiffuseIn;
in vec4 vSpecularIn;
out vec4 fragColor;

vec3 pick3(int src, vec3 mat, bool hasD, bool hasS) {
    if (src == 1 && hasD) return vDiffuseIn.rgb;
    if (src == 2 && hasS) return vSpecularIn.rgb;
    return mat;
}

// The Direct3D lighting model (Blinn-Phong, up to 8 lights), per pixel.
void lighting(vec3 P, vec3 N, bool hasD, bool hasS, out vec4 outDiffuse, out vec4 outSpec) {
    vec3 matD = pick3(uMatSrc.x, uMatDiffuse.rgb, hasD, hasS);
    vec3 matA = pick3(uMatSrc.y, uMatAmbient.rgb, hasD, hasS);
    vec3 matS = pick3(uMatSrc.z, uMatSpecular.rgb, hasD, hasS);
    vec3 matE = pick3(uMatSrc.w, uMatEmissive.rgb, hasD, hasS);
    float alpha = (uMatSrc.x == 1 && hasD) ? vDiffuseIn.a : ((uMatSrc.x == 2 && hasS) ? vSpecularIn.a : uMatDiffuse.a);
    vec3 amb = uGlobalAmbient.rgb * matA;
    vec3 dif = vec3(0.0);
    vec3 spec = vec3(0.0);
    bool doSpec = uLightFlags.z != 0;
    for (int i = 0; i < 8; ++i) {
        int type = int(uLights[i].pos.w);
        if (type == 0) continue;
        vec3 L;
        float att = 1.0;
        if (type == 3) {
            L = -normalize(uLights[i].dir.xyz);
        } else {
            vec3 d = uLights[i].pos.xyz - P;
            float dist = length(d);
            if (dist > uLights[i].dir.w) continue;
            L = d / max(dist, 1e-6);
            att = 1.0 / max(uLights[i].att.x + uLights[i].att.y * dist + uLights[i].att.z * dist * dist, 1e-6);
            if (type == 2) {
                float rho = dot(-L, normalize(uLights[i].dir.xyz));
                float cosPhi = uLights[i].spot.y, cosTheta = uLights[i].spot.x;
                if (rho <= cosPhi) continue;
                if (rho < cosTheta) att *= pow(clamp((rho - cosPhi) / max(cosTheta - cosPhi, 1e-6), 0.0, 1.0), uLights[i].att.w);
            }
        }
        amb += uLights[i].ambient.rgb * matA * att;
        float ndl = max(dot(N, L), 0.0);
        dif += uLights[i].diffuse.rgb * matD * ndl * att;
        if (doSpec && ndl > 0.0) {
            vec3 V = uLightFlags.w != 0 ? normalize(-P) : vec3(0.0, 0.0, -1.0);
            vec3 H = normalize(L + V);
            spec += uLights[i].specular.rgb * matS * pow(max(dot(N, H), 0.0), max(uGlobalAmbient.w, 1e-3)) * att;
        }
    }
    outDiffuse = vec4(clamp(amb + dif + matE, 0.0, 1.0), clamp(alpha, 0.0, 1.0));
    outSpec = vec4(clamp(spec, 0.0, 1.0), 0.0);
}

// Water: the texture ripples, the surface reflects the horizon (the fog colour) at grazing angles
// and shows the sun's glint.
vec2 waterRipple(vec2 uv) {
    float t = uEffect.y;
    return uv + 0.006 * vec2(sin(vPosWorld.x * 0.004 + vPosWorld.y * 0.0017 + t * 1.1), cos(vPosWorld.y * 0.0037 - vPosWorld.x * 0.0012 + t * 0.9));
}

vec3 waterSurface(vec3 base) {
    vec3 N = normalize(mat3(uView) * vec3(0.0, 0.0, 1.0));
    vec3 V = normalize(-vPosView);
    float facing = clamp(dot(N, V), 0.0, 1.0);
    float fresnel = pow(1.0 - facing, 4.0);
    vec3 color = mix(base, uFogColor.rgb, fresnel * 0.5);
    if (int(uLights[0].pos.w) == 3) {
        vec3 L = -normalize(uLights[0].dir.xyz);
        vec3 H = normalize(L + V);
        color += uLights[0].diffuse.rgb * pow(max(dot(N, H), 0.0), 160.0) * 0.6;
    }
    return color;
}

// Terrain detail: the tile texture's own brightness is read as relief and shades the tile slightly
// under the sun. Only each splat's colour is scaled, so the blend between tiles stays exact; past the
// fade distance the ratio is exactly 1 and the terrain looks as before.
float terrainRelief() {
    float k = uEffect.z * (1.0 - smoothstep(1200.0, 3000.0, vDepth));
    if (k <= 0.0 || int(uLights[0].pos.w) != 3) return 1.0;
    const vec3 LUMA = vec3(0.299, 0.587, 0.114);
    vec2 uv = vTex0.xy;
    vec2 e = max(vec2(1.0) / vec2(textureSize(uSampler0, 0)), fwidth(uv));
    float h = dot(texture(uSampler0, uv).rgb, LUMA);
    float du = clamp(dot(texture(uSampler0, uv + vec2(e.x, 0.0)).rgb, LUMA) - h, -0.25, 0.25);
    float dv = clamp(dot(texture(uSampler0, uv + vec2(0.0, e.y)).rgb, LUMA) - h, -0.25, 0.25);
    // The tile is mapped planar onto world x (u) and -y (v).
    vec3 N = normalize(vNormView);
    mat3 toView = mat3(uView * uWorld);
    vec3 T = toView * vec3(1.0, 0.0, 0.0);
    vec3 B = toView * vec3(0.0, -1.0, 0.0);
    T = normalize(T - N * dot(N, T));
    B = normalize(B - N * dot(N, B) - T * dot(T, B));
    // The first-order change of N.L for the bent normal: it averages to zero over the texture, so
    // the relief adds light and shade without darkening or brightening the ground.
    vec3 L = -normalize(uLights[0].dir.xyz);
    float ndl = dot(N, L);
    float change = -k * (du * dot(T, L) + dv * dot(B, L)) * smoothstep(0.0, 0.2, ndl);
    float A = dot(uLights[0].ambient.rgb + uGlobalAmbient.rgb, LUMA);
    float D = dot(uLights[0].diffuse.rgb, LUMA);
    float r = 1.0 + D * change / max(A + D * max(ndl, 0.0), 1e-3);
    return clamp(r, 0.8, 1.2);
}

vec4 arg(int a, vec4 cur, vec4 tex, vec4 dif, vec4 spec) {
    int sel = a & 15;
    vec4 c = sel == 0 ? dif : (sel == 1 ? cur : (sel == 2 ? tex : (sel == 3 ? uTFactor : spec)));
    if ((a & 16) != 0) c = vec4(1.0) - c;
    if ((a & 32) != 0) c = vec4(c.a);
    return c;
}

vec4 op(int o, vec4 a1, vec4 a2, vec4 cur, vec4 tex, vec4 dif) {
    switch (o) {
        case 2: return a1;
        case 3: return a2;
        case 4: return a1 * a2;
        case 5: return a1 * a2 * 2.0;
        case 6: return a1 * a2 * 4.0;
        case 7: return a1 + a2;
        case 8: return a1 + a2 - 0.5;
        case 9: return (a1 + a2 - 0.5) * 2.0;
        case 10: return a1 - a2;
        case 11: return a1 + a2 - a1 * a2;
        case 12: return mix(a2, a1, dif.a);
        case 13: return mix(a2, a1, tex.a);
        case 14: return mix(a2, a1, uTFactor.a);
        case 15: return a1 + a2 * (1.0 - tex.a);
        case 16: return mix(a2, a1, cur.a);
        case 17: return a1;
        case 18: return vec4(a1.rgb + a1.a * a2.rgb, a1.a);
        case 19: return vec4(a1.rgb * a2.rgb + a1.a, a1.a);
        case 20: return vec4((1.0 - a1.a) * a2.rgb + a1.rgb, a1.a);
        case 21: return vec4((1.0 - a1.rgb) * a2.rgb + a1.a, a1.a);
        case 24: return vec4(4.0 * dot(a1.rgb - 0.5, a2.rgb - 0.5));
        default: return a1;
    }
}

vec4 sampleStage(int s) {
    vec4 tc = s == 0 ? vTex0 : vTex1;
    vec2 uv = tc.xy;
    if (uStageC[s].w != 0) uv /= (uStageC[s].x == 3 ? tc.z : tc.w);
    if (uEffect.x == 1.0) uv = waterRipple(uv);
    return s == 0 ? texture(uSampler0, uv) : texture(uSampler1, uv);
}

void main() {
    vec4 dif = vColor;
    vec4 specIn = vSpec;
    if (uLightFlags.x != 0 && uMisc.y == 0.0) {
        vec3 N = normalize(vNormView);
        lighting(vPosView, N, uMisc.z != 0.0, uMisc.w != 0.0, dif, specIn);
    }
    vec4 cur = dif;
    for (int s = 0; s < 2; ++s) {
        int colorOp = uStageA[s].x;
        if (colorOp == 1) break;
        vec4 tex = sampleStage(s);
        if (s == 0 && uEffect.x == 2.0) tex.rgb *= terrainRelief();
        vec4 c1 = arg(uStageA[s].y, cur, tex, dif, specIn);
        vec4 c2 = arg(uStageA[s].z, cur, tex, dif, specIn);
        vec4 res = op(colorOp, c1, c2, cur, tex, dif);
        int alphaOp = uStageA[s].w;
        if (alphaOp == 1) {
            res.a = cur.a;
        } else {
            vec4 a1 = arg(uStageB[s].x, cur, tex, dif, specIn);
            vec4 a2 = arg(uStageB[s].y, cur, tex, dif, specIn);
            res.a = op(alphaOp, a1, a2, cur, tex, dif).a;
        }
        cur = clamp(res, 0.0, 1.0);
    }
    if (uLightFlags.z != 0) cur.rgb = clamp(cur.rgb + specIn.rgb, 0.0, 1.0);
    if (uEffect.x == 1.0) cur.rgb = clamp(waterSurface(cur.rgb), 0.0, 1.0);
    if (uAlpha.x != 0) {
        float ref = uMisc.x;
        float a = cur.a;
        int f = uAlpha.y;
        bool pass = true;
        if (f == 1) pass = false;
        else if (f == 2) pass = a < ref;
        else if (f == 3) pass = abs(a - ref) < 0.002;
        else if (f == 4) pass = a <= ref;
        else if (f == 5) pass = a > ref;
        else if (f == 6) pass = abs(a - ref) >= 0.002;
        else if (f == 7) pass = a >= ref;
        if (!pass) discard;
    }
    if (uFogI.x != 0) {
        // Vertex fog is evaluated here per pixel as well: the terrain patches are large and the
        // bands of per-vertex fog showed.
        float f = vFog;
        int mode = uFogI.z != 0 ? uFogI.z : uFogI.y;
        if (vDepth >= 0.0 && mode != 0) {
            float d = vDepth;
            if (mode == 3) f = clamp((uFogP.y - d) / max(uFogP.y - uFogP.x, 1e-6), 0.0, 1.0);
            else if (mode == 1) f = exp(-uFogP.z * d);
            else if (mode == 2) { float e = uFogP.z * d; f = exp(-e * e); }
        }
        cur.rgb = mix(uFogColor.rgb, cur.rgb, clamp(f, 0.0, 1.0));
    }
    fragColor = cur;
}
)GLSL";
    // clang-format on
}
