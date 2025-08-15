#include <util/util.hpp>

namespace 
{
    
}

glm::mat4 interpolateMatrix(const glm::mat4& start, const glm::mat4& end, float alpha) {

    glm::vec3 scaleA, transA, skewA;
    glm::vec4 perspectiveA;
    glm::quat rotA;
    glm::decompose(start, scaleA, rotA, transA, skewA, perspectiveA);

    glm::vec3 scaleB, transB, skewB;
    glm::vec4 perspectiveB;
    glm::quat rotB;
    glm::decompose(end, scaleB, rotB, transB, skewB, perspectiveB);

    glm::vec3 interpTrans = glm::mix(transA, transB, alpha);
    glm::vec3 interpScale = glm::mix(scaleA, scaleB, alpha);
    glm::quat interpRot = glm::slerp(rotA, rotB, alpha);

    glm::mat4 result = glm::translate(glm::mat4(1.0f), interpTrans) * glm::mat4_cast(interpRot) * glm::scale(glm::mat4(1.0f), interpScale);
    return result;
}