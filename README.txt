Engine was built on a graphics engine by Niko Procopi, which can be found here:
https://github.com/IGME-RIT/VkHitboxes

This physics engine attempts to implement simultaneous collision resolution with
resting contacts. This relies on an Linear Complementary Problem solver in order
to (attempt) to solve all of the collisions in the scene at once. Collisions were
detected with a modified SAT to determine multiple points of collisions. The 
following are sources I used quite a bit for this project (they are also linked 
in code comments):

Collision Detection GDC talk by Dirk Gregorius
http://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf

Game Physics, 2nd Edition by David Eberly
https://www.amazon.com/Game-Physics-David-H-Eberly/dp/0123749034

Geometric Tools (used their LCP solver)
https://www.geometrictools.com/
