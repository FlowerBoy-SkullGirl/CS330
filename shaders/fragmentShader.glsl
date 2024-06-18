#version 440 core

struct Material 
{
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct LightSource 
{
    bool enabled;
    vec3 position;	
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float focalStrength;
    float specularIntensity;
};

#define TOTAL_LIGHTS 4

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

out vec4 outFragmentColor;

uniform bool bUseTexture=false;
uniform bool bUseTexture2=false;
uniform bool bUseSpecularMap=false;
uniform bool bUseLighting=false;
uniform vec4 objectColor = vec4(1.0f);
uniform sampler2D objectTexture;
uniform sampler2D objectTexture2;
uniform sampler2D specularMap;
uniform vec3 viewPosition;
uniform vec2 UVscale = vec2(1.0f, 1.0f);
uniform LightSource lightSources[TOTAL_LIGHTS];
uniform Material material;
uniform vec3 globalAmbientColor;
    

// function prototypes
vec3 CalcLightSource(LightSource light, vec3 lightNormal, vec3 vertexPosition, vec3 viewDirection);

void main()
{
   if(bUseLighting == true)
   {
      // properties
      vec3 lightNormal = normalize(fragmentVertexNormal);
      vec3 viewDirection = normalize(viewPosition - fragmentPosition);
      vec3 phongResult = vec3(0.0f);

      for(int i = 0; i < TOTAL_LIGHTS; i++)
      {
         phongResult += CalcLightSource(lightSources[i], lightNormal, fragmentPosition, viewDirection); 
      }   
    
      if(bUseTexture == true)
      {
         vec4 textureColor;
         if (bUseTexture2 == true){
           textureColor = mix(
	         texture(objectTexture, fragmentTextureCoordinate * UVscale),
	         texture(objectTexture2, fragmentTextureCoordinate * UVscale),
	         0.2);
	     }else{
           textureColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
	     }
         outFragmentColor = vec4(phongResult * textureColor.xyz, 1.0);
      }
      else
      {
         outFragmentColor = vec4(phongResult * objectColor.xyz, objectColor.w);
      }
   }
   else 
   {
      if(bUseTexture == true)
      {
	    if(bUseTexture2 == true){
	      outFragmentColor = mix(
	         texture(objectTexture, fragmentTextureCoordinate * UVscale),
	         texture(objectTexture2, fragmentTextureCoordinate * UVscale),
	         0.2);
	    }else{
          outFragmentColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
	    }
      }
      else
      {
         outFragmentColor = objectColor;
      }
   }
}

// calculates the color when using a directional light.
vec3 CalcLightSource(LightSource light, vec3 lightNormal, vec3 vertexPosition, vec3 viewDirection)
{
   if (!light.enabled){
   	return vec3(0.0, 0.0, 0.0);
   }

   vec3 ambient;
   vec3 diffuse;
   vec3 specular;

   //**Calculate Ambient lighting**

   ambient = globalAmbientColor * light.ambientColor;

   //**Calculate Diffuse lighting**

   // Calculate distance (light direction) between light source and fragments/pixels
   vec3 lightDirection = normalize(light.position - vertexPosition); 
   // Calculate diffuse impact by generating dot product of normal and light
   float impact = max(dot(lightNormal, lightDirection), 0.0);
   // Generate diffuse material color   
   diffuse = impact * material.diffuseColor * light.diffuseColor; 

   //**Calculate Specular lighting**

   // Calculate reflection vector
   vec3 reflectDir = reflect(-lightDirection, lightNormal);
   // Calculate specular component
   float specularComponent = pow(max(dot(viewDirection, reflectDir), 0.0), light.focalStrength);
   if (bUseSpecularMap){
        vec3 mapped_intensity = texture(specularMap, fragmentTextureCoordinate).xyz;
	specular = (light.specularIntensity * material.shininess) * specularComponent * material.specularColor * light.specularColor * mapped_intensity;
   }else{
	specular = (light.specularIntensity * material.shininess) * specularComponent * material.specularColor * light.specularColor;
   }
  
   return(ambient + diffuse + specular);
}
