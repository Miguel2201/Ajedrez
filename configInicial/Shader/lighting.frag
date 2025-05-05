#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
// Tal vez 'in vec3 Normal;' y 'in vec3 FragPos;' si los usas para algo más,
// pero probablemente ya no los necesites si quitas toda la luz.

uniform sampler2D texture_diffuse1; // Asumiendo que este es tu sampler de textura
uniform vec3 highlightColor;     // El color que envías desde C++

void main()
{
    // 1. Obtener color base
    vec4 baseColor = texture(texture_diffuse1, TexCoords);

    // 2. Aplicar resaltado (si highlightColor no es negro)
    // Puedes sumarlo, mezclarlo, etc. Mezclar es común:
    float highlightIntensity = step(0.01, max(highlightColor.r, max(highlightColor.g, highlightColor.b))); // Es > 0?
    vec3 finalColor = mix(baseColor.rgb, highlightColor, highlightIntensity * 0.5); // Mezcla al 50% si está activo

    // O una suma simple (puede saturar a blanco):
    // vec3 finalColor = baseColor.rgb + highlightColor;

    FragColor = vec4(finalColor, baseColor.a); // Mantener el alfa original
}
