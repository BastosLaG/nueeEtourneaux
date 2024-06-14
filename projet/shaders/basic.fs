#version 330

uniform vec4 couleur, lumpos;
uniform int id, nb_mobiles;
uniform sampler2D smTex;
in vec4 vsoNormal;
in vec4 vsoMVPos;
in vec4 vsoSMCoord;
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 fragId;

void main() {
    // Lumière positionnelle
    if(id == 2) {
        fragColor = vec4(1., 1., 0.5, 1.);
    } 
    // Ombre portée
    else if (id == 1) {
        vec3 N = normalize(vsoNormal.xyz);
        vec3 L = normalize(vsoMVPos.xyz - lumpos.xyz);
        vec3 projCoords = vsoSMCoord.xyz / vsoSMCoord.w;
        float diffuse = dot(N, -L);
        if(texture(smTex, projCoords.xy).r  <  projCoords.z) diffuse *= .6; 
        fragColor = vec4((couleur.rgb * diffuse), 0.1);
    } 
    // Phong
    else {
        vec3 ambient = couleur.rgb;
        vec3 diffuseColor = couleur.rgb;
        vec3 specularColor = vec3(0.5, 0.1, 0.9);
        float shininess = 32.0;

        vec3 N = normalize(vsoNormal.xyz);
        vec3 L = normalize(vsoMVPos.xyz - lumpos.xyz);
        vec3 V = normalize(-vsoMVPos.xyz);
        vec3 R = reflect(normalize(L), N);

        float diffuse = max(dot(N, -L), 0.0);
        vec3 diffuseComponent = diffuse * diffuseColor;

        float specular = pow(max(dot(R, V), 0.0), shininess);
        vec3 specularComponent = specular * specularColor;

        fragColor = vec4(ambient + diffuseComponent + specularComponent, couleur.a);
    }
    fragId = vec4(float(id) / (float(nb_mobiles) + 2.0));
}
