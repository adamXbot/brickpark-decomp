/* LEGOLAND path-square list and path-tile maintenance. */
#include "legoland.h"

typedef struct PathSquare {
    struct PathSquare* next; /* +0x00 */
    int pad4;                /* +0x04 */
    Rect rect;               /* +0x08 */
    int distance2;           /* +0x1c */
    int flags;               /* +0x20 */
} PathSquare;

typedef struct PathRect {
    int left;
    int top;
    int right;
    int bottom;
} PathRect;

extern PathSquare* g_path_squares; /* 0x0066b44c */
extern PathSquare* g_path_square_neighbours[]; /* 0x0066a45c */

extern void* HeapAlloc_w(unsigned int size); /* 0x0049e4ff */
extern void HeapFree_w(void* ptr);           /* 0x0049e4d0 */
extern void CollectPathSquareNeighbours(Rect* bounds); /* 0x00481810 */
extern void AddPathTileGFX(Pos* pos, unsigned short tile); /* 0x0045d350 */
extern void AddPathSquare(Pos* pos);                    /* 0x00481c50 */
extern int* g_path_tile_base;                           /* 0x00832bf0 */
extern void AdjustPathTile(Pos* pos, int tile);          /* 0x0045d1a0 */
extern int FindPathRect(Pos* pos, PathRect* rect);       /* 0x0045ca90 */
extern void GrowPathRect(PathRect* rect);                /* 0x0045cd00 */
extern void PaintPathRect(PathRect* rect);               /* 0x0045cb20 */

// FUNCTION: LEGOLAND 0x00481730
PathSquare* NewPathSquare(void)
{
    PathSquare* square = (PathSquare*)HeapAlloc_w(0x24);

    square->flags = 0;
    square->distance2 = 0;
    square->next = g_path_squares;
    g_path_squares = square;
    return square;
}

// FUNCTION: LEGOLAND 0x00481750
void FreePathSquare(PathSquare* square)
{
    PathSquare* prev = g_path_squares;

    if (prev == square) {
        g_path_squares = square->next;
        HeapFree_w(square);
        return;
    }

    while (prev->next != 0) {
        if (prev->next == square) {
            prev->next = square->next;
            HeapFree_w(square);
            return;
        }
        prev = prev->next;
    }
}

// FUNCTION: LEGOLAND 0x00481790
PathSquare* FindPathSquare(Pos* pos)
{
    PathSquare* square = g_path_squares;

    while (square != 0) {
        if (pos->x >= square->rect.left && pos->x <= square->rect.right &&
            pos->y >= square->rect.top && pos->y <= square->rect.bottom)
            return square;
        square = square->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00481b10
void PathSquareAdded(PathSquare* square)
{
    int i;
    Rect* rect;

again:
    rect = &square->rect;
    CollectPathSquareNeighbours(rect);

    i = 0;
    while (g_path_square_neighbours[i] != 0) {
        if (g_path_square_neighbours[i]->rect.left == rect->left &&
            g_path_square_neighbours[i]->rect.right == square->rect.right) {
            g_path_square_neighbours[i]->rect.bottom = square->rect.bottom;
            goto merged;
        }
        ++i;
    }

    ++i;
    while (g_path_square_neighbours[i] != 0) {
        if (g_path_square_neighbours[i]->rect.left == rect->left &&
            g_path_square_neighbours[i]->rect.right == square->rect.right) {
            g_path_square_neighbours[i]->rect.top = square->rect.top;
            goto merged;
        }
        ++i;
    }

    ++i;
    while (g_path_square_neighbours[i] != 0) {
        if (g_path_square_neighbours[i]->rect.top == square->rect.top &&
            g_path_square_neighbours[i]->rect.bottom == square->rect.bottom) {
            g_path_square_neighbours[i]->rect.right = square->rect.right;
            goto merged;
        }
        ++i;
    }

    ++i;
    if (g_path_square_neighbours[i] == 0)
        goto done;
    do {
        if (g_path_square_neighbours[i]->rect.top == square->rect.top &&
            g_path_square_neighbours[i]->rect.bottom == square->rect.bottom) {
            g_path_square_neighbours[i]->rect.left = rect->left;
            goto merged;
        }
        ++i;
    } while (g_path_square_neighbours[i] != 0);
done:
    return;

merged:
    FreePathSquare(square);
    square = g_path_square_neighbours[i];
    goto again;
}

// FUNCTION: LEGOLAND 0x00481c90
void RemovePathSquare(Pos* pos)
{
    PathSquare* square;
    Rect old;

    square = FindPathSquare(pos);
    if (square != 0) {
        old = square->rect;
        FreePathSquare(square);

        if (old.top < pos->y) {
            square = NewPathSquare();
            square->rect.top = old.top;
            square->rect.bottom = pos->y - 1;
            square->rect.left = old.left;
            square->rect.right = old.right;
            PathSquareAdded(square);
        }

        if (old.bottom > pos->y) {
            square = NewPathSquare();
            square->rect.top = pos->y + 1;
            square->rect.bottom = old.bottom;
            square->rect.left = old.left;
            square->rect.right = old.right;
            PathSquareAdded(square);
        }

        if (old.left < pos->x) {
            square = NewPathSquare();
            square->rect.top = square->rect.bottom = pos->y;
            square->rect.left = old.left;
            square->rect.right = pos->x - 1;
            PathSquareAdded(square);
        }

        if (old.right > pos->x) {
            square = NewPathSquare();
            square->rect.top = square->rect.bottom = pos->y;
            square->rect.left = pos->x + 1;
            square->rect.right = old.right;
            PathSquareAdded(square);
        }
    }
}

// FUNCTION: LEGOLAND 0x0045cd30
void RefreshPathSquare(Pos* pos)
{
    PathRect rect;

    if (FindPathRect(pos, &rect)) {
        GrowPathRect(&rect);
        PaintPathRect(&rect);
    }
}

// FUNCTION: LEGOLAND 0x0045d260
void UpdatePathNeighbours(Pos* pos)
{
    Pos neighbour;
    int tile = *g_path_tile_base;

    AdjustPathTile(pos, tile);

    neighbour.x = pos->x;
    neighbour.y = pos->y - 1;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x + 1;
    neighbour.y = pos->y;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x;
    neighbour.y = pos->y + 1;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x - 1;
    neighbour.y = pos->y;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x - 1;
    neighbour.y = pos->y + 1;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x + 1;
    neighbour.y = pos->y - 1;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x + 1;
    neighbour.y = pos->y + 1;
    AdjustPathTile(&neighbour, tile);

    neighbour.x = pos->x - 1;
    neighbour.y = pos->y - 1;
    AdjustPathTile(&neighbour, tile);
}

// FUNCTION: LEGOLAND 0x0045d3b0
void AddPathTile(Pos* pos, unsigned short tile)
{
    AddPathTileGFX(pos, tile);
    AddPathSquare(pos);
}
