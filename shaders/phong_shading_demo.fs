#version 330 core

// Data from vertex shader.
// --------------------------------------------------------
in vec3 iNormalWorld;
in vec3 iPosWorld;
in vec2 iTexCoord;

// Add your data for interpolation.
// --------------------------------------------------------
out vec4 FragColor;

// --------------------------------------------------------
// Add your uniform variables.
// --------------------------------------------------------

// Material data
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;
uniform sampler2D mapKd;
uniform int flag;

// Light data
uniform vec3 ambientLight;
uniform vec3 dirLightDir;
uniform vec3 dirLightRadiance;
uniform vec3 pointLightPos;
uniform vec3 pointLightIntensity;
uniform vec3 spotLightPos;
uniform vec3 spotLightIntensity;
uniform vec3 spotLightDir;
uniform float spotLightCutoffStartDeg;
uniform float spotLightTotalWidthDeg;

// camera
uniform vec3 cameraPos;

vec3 Diffuse(vec3 Kd, vec3 I, vec3 N, vec3 lightDir){
    return Kd * I * max(0.0, dot(N, lightDir));
}

vec3 Specular(vec3 Ks, vec3 I, vec3 N, vec3 halfDir, float Ns){
    return Ks * I * pow(max(dot(N, halfDir), 0.0), Ns);
}

void main()
{
    // --------------------------------------------------------
    // Add your implementation.
    // --------------------------------------------------------
    vec3 N = normalize(iNormalWorld);

    vec3 texColor = texture2D(mapKd, iTexCoord).rgb;
    vec3 objKd = Kd;
    if (flag == 1) {
        objKd = texColor;
    }
    
    // Ambient light
    vec3 ambient = Ka * ambientLight;

    // Directional light
    vec3 lightDir = normalize(-dirLightDir);
    vec3 diffuse = Diffuse(objKd, dirLightRadiance, N, lightDir);
    vec3 viewDir = normalize(cameraPos - iPosWorld);
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 specular = Specular(Ks, dirLightRadiance, N, halfDir, Ns);
    vec3 dirLight = diffuse + specular;
    
    // Point light
    lightDir = normalize(pointLightPos - iPosWorld);
    float distSurfaceToLight = distance(pointLightPos, iPosWorld);
    float attenuation = 1.0f / (distSurfaceToLight * distSurfaceToLight);
    vec3 radiance = pointLightIntensity * attenuation;
    diffuse = Diffuse(objKd, radiance, N, lightDir);
    halfDir = normalize(lightDir + viewDir);
    specular = Specular(Ks, radiance, N, halfDir, Ns);
    vec3 pointLight = diffuse + specular;

    // Spot light
    lightDir = normalize(spotLightPos - iPosWorld);
    float theta = dot(lightDir, normalize(-spotLightDir));
    float cosRadiansCutoffDeg = cos(radians(spotLightCutoffStartDeg));
    float cosRadiansTotalWidthDeg = cos(radians(spotLightTotalWidthDeg));
    float epsilon = cosRadiansCutoffDeg - cosRadiansTotalWidthDeg;
    float intensity = clamp((theta-cosRadiansTotalWidthDeg) / epsilon, 0.0, 1.0);
    distSurfaceToLight = distance(spotLightPos, iPosWorld);
    attenuation = 1.0f / (distSurfaceToLight * distSurfaceToLight);
    diffuse = Diffuse(objKd, spotLightIntensity, N, lightDir);
    halfDir = normalize(lightDir + viewDir);
    specular = Specular(Ks, spotLightIntensity, N, halfDir, Ns);
    diffuse *= intensity * attenuation;
    specular *= intensity * attenuation;
    vec3 spotLight = diffuse + specular;

    vec3 pxColor = ambient + dirLight + pointLight + spotLight;
    FragColor = vec4(pxColor, 1.0);
}