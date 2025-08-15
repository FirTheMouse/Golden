#include <duel/spellForm.hpp>
#include <duel/turnManager.hpp>


namespace Golden
{

SpellForm& SpellForm::moveAnim(const vec3& delta)
{
    if (scene) {
            glm::mat4 to = getTransform();
            to[3] = glm::vec4(glm::vec3(to[3])+delta.toGlm(), 1.0f);
            GDM::get_T().requestMove(this, to);
            if(s_type()==S_TYPE::INSTANCED) {requestInstanceUpdate();}
    }
    return *this;
}

SpellForm& SpellForm::scaleAnim(float x,float y,float z)
{
    if (scene) {
        glm::mat4 mat = getTransform();

        glm::vec3 right   = glm::normalize(glm::vec3(mat[0]));
        glm::vec3 up      = glm::normalize(glm::vec3(mat[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(mat[2]));

        mat[0] = glm::vec4(right * x, 0.0f);
        mat[1] = glm::vec4(up * y, 0.0f);
        mat[2] = glm::vec4(forward * z, 0.0f);

        GDM::get_T().requestMove(this, mat);
        if(s_type()==S_TYPE::INSTANCED) {requestInstanceUpdate();}
    }
    return *this;
}

}