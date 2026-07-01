#pragma once

#include <glm/glm.hpp>

#include <vector>

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

// Strict overlap: faces merely touching do not count.
inline bool aabbOverlap(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

struct WorldBox {
    AABB box;
    glm::vec3 color{1.0f};
    bool checkerTop = false; // draw a checker pattern on upward faces
};

class World {
public:
    void buildTestMap();
    bool overlapsAny(const AABB& box) const;
    const std::vector<WorldBox>& boxes() const { return boxes_; }

    glm::vec3 spawnPoint{0.0f, 0.01f, 0.0f};

private:
    void add(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool checkerTop = false);
    std::vector<WorldBox> boxes_;
};
