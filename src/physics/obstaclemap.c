/*
 * Open Surge Engine
 * obstaclemap.c - physics system: obstacle map
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../core/video.h"
#include "../util/darray.h"
#include "../util/util.h"

/* an obstacle map is just a set of obstacles */
struct obstaclemap_t
{
    DARRAY(const obstacle_t*, obstacle);
};

/* private methods */
static const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm);
static inline bool ignore_obstacle(const obstacle_t *obstacle, obstaclelayer_t layer_filter);

/* public methods */
obstaclemap_t* obstaclemap_create()
{
    obstaclemap_t *obstaclemap = mallocx(sizeof *obstaclemap);
    darray_init_ex(obstaclemap->obstacle, 32);
    return obstaclemap;
}

obstaclemap_t* obstaclemap_destroy(obstaclemap_t *obstaclemap)
{
    darray_release(obstaclemap->obstacle);
    free(obstaclemap);
    return NULL;
}

void obstaclemap_add_obstacle(obstaclemap_t *obstaclemap, const obstacle_t *obstacle)
{
    darray_push(obstaclemap->obstacle, obstacle);
}

const obstacle_t* obstaclemap_get_best_obstacle_at(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm, obstaclelayer_t layer_filter)
{
    const obstacle_t *best = NULL;

    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        const obstacle_t *obstacle = obstaclemap->obstacle[i];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x1, y1, x2, y2))
            best = pick_best_obstacle(obstacle, best, x1, y1, x2, y2, mm);
    }

    return best;
}

bool obstaclemap_obstacle_exists(const obstaclemap_t* obstaclemap, int x, int y, obstaclelayer_t layer_filter)
{
    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        const obstacle_t *obstacle = obstaclemap->obstacle[i];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x, y, x, y))
            return true;
    }

    return false;
}

bool obstaclemap_solid_exists(const obstaclemap_t* obstaclemap, int x, int y, obstaclelayer_t layer_filter)
{
    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        const obstacle_t *obstacle = obstaclemap->obstacle[i];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x, y, x, y) && obstacle_is_solid(obstacle))
            return true;
    }

    return false;
}

/* removes all obstacles from the obstacle map */
void obstaclemap_clear(obstaclemap_t* obstaclemap)
{
    darray_clear(obstaclemap->obstacle);
}

/* private methods */

/* considering that a and b overlap, which one should we pick? */
/* we know that x1 <= x2 and y1 <= y2; these values already come rotated according to the movmode */
const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm)
{
    int x, y, ha, hb;

    /* NULL pointers should be handled */
    if(a == NULL)
        return b;
    if(b == NULL)
        return a;

    /* solid obstacles are more preferable than one-way platforms */
    if(!obstacle_is_solid(a) && obstacle_is_solid(b))
        return b;
    if(!obstacle_is_solid(b) && obstacle_is_solid(a))
        return a;

    #if 1
    /* one-way platforms only: get the shortest obstacle */
    if(!obstacle_is_solid(a) && !obstacle_is_solid(b)) {
        switch(mm) {
            case MM_FLOOR:
                ha = obstacle_ground_position(a, x2, y2, GD_DOWN);
                hb = obstacle_ground_position(b, x2, y2, GD_DOWN);
                return ha >= hb ? a : b;

            case MM_RIGHTWALL:
                ha = obstacle_ground_position(a, x2, y2, GD_RIGHT);
                hb = obstacle_ground_position(b, x2, y2, GD_RIGHT);
                return ha >= hb ? a : b;

            case MM_CEILING:
                ha = obstacle_ground_position(a, x2, y1, GD_UP);
                hb = obstacle_ground_position(b, x2, y1, GD_UP);
                return ha < hb ? a : b;
                
            case MM_LEFTWALL:
                ha = obstacle_ground_position(a, x1, y2, GD_LEFT);
                hb = obstacle_ground_position(b, x1, y2, GD_LEFT);
                return ha < hb ? a : b;
        }
    }
    #endif

    /* get the tallest obstacle */
    switch(mm) {
        case MM_FLOOR:
            x = x2; /* x1 == x2 */
            y = y2; /* y2 == max(y1, y2) */
            ha = obstacle_ground_position(a, x, y, GD_DOWN);
            hb = obstacle_ground_position(b, x, y, GD_DOWN);
            return ha < hb ? a : b;

        case MM_LEFTWALL:
            x = x1; /* x1 == min(x1, x2) */
            y = y2; /* y1 == y2 */
            ha = obstacle_ground_position(a, x, y, GD_LEFT);
            hb = obstacle_ground_position(b, x, y, GD_LEFT);
            return ha >= hb ? a : b;

        case MM_CEILING:
            x = x2; /* x1 == x2 */
            y = y1; /* y1 == min(y1, y2) */
            ha = obstacle_ground_position(a, x, y, GD_UP);
            hb = obstacle_ground_position(b, x, y, GD_UP);
            return ha >= hb ? a : b;

        case MM_RIGHTWALL:
            x = x2; /* x2 == max(x1, x2) */
            y = y2; /* y1 == y2 */
            ha = obstacle_ground_position(a, x, y, GD_RIGHT);
            hb = obstacle_ground_position(b, x, y, GD_RIGHT);
            return ha < hb ? a : b;
    }

    /* this shouldn't happen */
    return a;
}

/* whether or not the given obstacle should be ignored, given a layer filter */
bool ignore_obstacle(const obstacle_t *obstacle, obstaclelayer_t layer_filter)
{
    obstaclelayer_t obstacle_layer = obstacle_get_layer(obstacle);
    return layer_filter != OL_DEFAULT && obstacle_layer != OL_DEFAULT && obstacle_layer != layer_filter;
}