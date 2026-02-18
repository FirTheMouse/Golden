#pragma once
#include<core/helper.hpp>
#include<gui/twig.hpp>

void make_vstack(g_ptr<Quad> container, float spacing = 5.0f) {
    for(int i = 0; i < container->children.length(); i++) {
        auto child = container->children[i];
        int index = i;
        child->joint = [child, container, index, spacing]() {
            if(index == 0) {
                child->position = container->position;
            } else {
                auto prev = container->children[index - 1];
                child->position = vec2(prev->position.x(), prev->position.y() + prev->scaleVec.y() + spacing);
            }
            return true;
        };
    }
}

void make_hstack(g_ptr<Quad> container, float spacing = 5.0f) {
    for(int i = 0; i < container->children.length(); i++) {
        auto child = container->children[i];
        int index = i;
        child->joint = [child, container, index, spacing]() {
            if(index == 0) {
                child->position = container->position;
            } else {
                auto prev = container->children[index - 1];
                child->position = vec2(prev->position.x() + prev->scaleVec.x() + spacing, prev->position.y());
            }
            return true;
        };
    }
}

void make_autosize(g_ptr<Quad> container, float padding = 10.0f) {
    container->joint = [container, padding]() {
        if(container->parent)
            container->position = container->parent->position;

        float max_x = 0, max_y = 0;
        for(auto child : container->allChildren()) {
            if(!child->isAnchor&&child->isSelectable) continue;
            if(!child->isActive()) continue;
            vec2 child_end = child->position + child->getScale() - container->position;
            max_x = std::max(max_x, child_end.x());
            max_y = std::max(max_y, child_end.y());
        }
        if(!container->children.empty()) {
            container->scaleVec = vec2(max_x + padding * 2, max_y + padding * 2);
        }
        
        return true;
    };
}

void autosize(g_ptr<Quad> container, float padding = 10.0f) {
        float max_x = 0, max_y = 0;
        for(auto child : container->allChildren()) {
            if(!child->isAnchor&&child->isSelectable) continue;
            if(!child->isActive()) continue;
            vec2 child_end = child->position + child->getScale() - container->position;
            max_x = std::max(max_x, child_end.x());
            max_y = std::max(max_y, child_end.y());
        }
        if(!container->children.empty()) {
            container->setScale(vec2(max_x + padding * 2, max_y + padding * 2));
        }
}

g_ptr<Quad> make_panel(g_ptr<Object> obj) {
    if(obj->has("gui_func")) {
        return obj->get<std::function<g_ptr<Quad>()>>("gui_func")();
    }
    return nullptr;
}