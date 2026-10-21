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
    Light uLights[8];
};

out vec4 vColor;
out vec4 vSpec;
out vec4 vTex0;
out vec4 vTex1;
out float vFog;
out float vDepth;

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

vec3 pick3(int src, vec3 mat, bool hasD, bool hasS) {
    if (src == 1 && hasD) return aDiffuse.rgb;
    if (src == 2 && hasS) return aSpecular.rgb;
    return mat;
}

void lighting(vec3 P, vec3 N, bool hasD, bool hasS, out vec4 outDiffuse, out vec4 outSpec) {
    vec3 matD = pick3(uMatSrc.x, uMatDiffuse.rgb, hasD, hasS);
    vec3 matA = pick3(uMatSrc.y, uMatAmbient.rgb, hasD, hasS);
    vec3 matS = pick3(uMatSrc.z, uMatSpecular.rgb, hasD, hasS);
    vec3 matE = pick3(uMatSrc.w, uMatEmissive.rgb, hasD, hasS);
    float alpha = (uMatSrc.x == 1 && hasD) ? aDiffuse.a : ((uMatSrc.x == 2 && hasS) ? aSpecular.a : uMatDiffuse.a);
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

float fogFactor(float d) {
    int mode = uFogI.y;
    if (mode == 3) return clamp((uFogP.y - d) / max(uFogP.y - uFogP.x, 1e-6), 0.0, 1.0);
    if (mode == 1) return exp(-uFogP.z * d);
    if (mode == 2) { float e = uFogP.z * d; return exp(-e * e); }
    return 1.0;
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
        vDepth = aPos.z;
    } else {
        vec4 wp = uWorld * vec4(aPos.xyz, 1.0);
        vec4 vp = uView * wp;
        P = vp.xyz;
        N = normalize(mat3(uNormal) * aNormal);
        clip = uProj * vp;
        clip.xy += vec2(uViewport.w / uViewport.x, -uViewport.w / uViewport.y) * clip.w;
        if (uLightFlags.x != 0) {
            vec4 d, s;
            lighting(P, N, hasD, hasS, d, s);
            vColor = d;
            vSpec = s;
        } else {
            vColor = hasD ? aDiffuse : vec4(1.0);
            vSpec = hasS ? aSpecular : vec4(0.0);
        }
        float fd = uFogI.w != 0 ? length(P) : abs(P.z);
        vDepth = fd;
        vFog = (uFogI.x != 0 && uFogI.z == 0) ? fogFactor(fd) : 1.0;
    }
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
out vec4 fragColor;

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
    return s == 0 ? texture(uSampler0, uv) : texture(uSampler1, uv);
}

void main() {
    vec4 dif = vColor;
    vec4 cur = dif;
    for (int s = 0; s < 2; ++s) {
        int colorOp = uStageA[s].x;
        if (colorOp == 1) break;
        vec4 tex = sampleStage(s);
        vec4 c1 = arg(uStageA[s].y, cur, tex, dif, vSpec);
        vec4 c2 = arg(uStageA[s].z, cur, tex, dif, vSpec);
        vec4 res = op(colorOp, c1, c2, cur, tex, dif);
        int alphaOp = uStageA[s].w;
        if (alphaOp == 1) {
            res.a = cur.a;
        } else {
            vec4 a1 = arg(uStageB[s].x, cur, tex, dif, vSpec);
            vec4 a2 = arg(uStageB[s].y, cur, tex, dif, vSpec);
            res.a = op(alphaOp, a1, a2, cur, tex, dif).a;
        }
        cur = clamp(res, 0.0, 1.0);
    }
    if (uLightFlags.z != 0) cur.rgb = clamp(cur.rgb + vSpec.rgb, 0.0, 1.0);
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
        float f = vFog;
        if (uFogI.z != 0) {
            int mode = uFogI.z;
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
