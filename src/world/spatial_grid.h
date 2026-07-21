// Uniform spatial hash over the zone's XZ plane. Everything placed in a
// zone (props, colliders) registers here, and every spatial query
// (collision resolve, camera boom, draw gathering, spacing checks) walks
// only nearby cells — huge zones stay O(local), never O(everything).
// Items spanning several cells are deduplicated with query stamps.
#pragma once
#include "raylib.h"
#include <algorithm>
#include <vector>

class SpatialGrid {
public:
    void Init(float halfExtent, float cellSize) {
        half = halfExtent;
        cell = cellSize;
        cols = (int)(halfExtent * 2.0f / cellSize) + 1;
        cells.assign((size_t)cols * cols, {});
        stamps.clear();
        queryId = 0;
    }

    void Insert(int index, Vector2 xz, float radius) {
        if ((int)stamps.size() <= index) stamps.resize(index + 1, 0);
        int x0, z0, x1, z1;
        Range(xz, radius, x0, z0, x1, z1);
        for (int z = z0; z <= z1; z++)
            for (int x = x0; x <= x1; x++)
                cells[(size_t)z * cols + x].push_back(index);
    }

    // fn(int index) — called exactly once per item near the circle.
    template <typename F>
    void Query(Vector2 center, float radius, F&& fn) const {
        queryId++;
        if (queryId == 0) {
            std::fill(stamps.begin(), stamps.end(), 0);
            queryId = 1;
        }
        int x0, z0, x1, z1;
        Range(center, radius, x0, z0, x1, z1);
        for (int z = z0; z <= z1; z++)
            for (int x = x0; x <= x1; x++)
                for (int idx : cells[(size_t)z * cols + x]) {
                    if (stamps[idx] == queryId) continue;
                    stamps[idx] = queryId;
                    fn(idx);
                }
    }

private:
    void Range(Vector2 c, float r, int& x0, int& z0, int& x1, int& z1) const {
        auto toCell = [&](float v) {
            int i = (int)((v + half) / cell);
            return i < 0 ? 0 : (i >= cols ? cols - 1 : i);
        };
        x0 = toCell(c.x - r); x1 = toCell(c.x + r);
        z0 = toCell(c.y - r); z1 = toCell(c.y + r);
    }

    std::vector<std::vector<int>> cells;
    mutable std::vector<unsigned int> stamps;
    mutable unsigned int queryId = 0;
    float half = 0, cell = 16.0f;
    int cols = 0;
};
