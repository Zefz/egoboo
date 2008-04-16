#include "Physics.h"
#include "mathstuff.h"

//--------------------------------------------------------------------------------------------
bool interact(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z)
{
  float dx,dy,dist;

  if(0==ritema.bump_height || 0==ritemb.bump_height) return false;

  // First check absolute value diamond
  dx = ABS(ritema.pos.x-ritemb.pos.x);
  dy = ABS(ritema.pos.y-ritemb.pos.y);
  dist = dx + dy;
  if (dist > ABS(ritema.calc_bump_size_big) + ABS(ritemb.calc_bump_size_big)) return false;
  if( dx > ABS(ritema.calc_bump_size) + ABS(ritemb.calc_bump_size) ) return false;
  if( dy > ABS(ritema.calc_bump_size) + ABS(ritemb.calc_bump_size) ) return false;

  if(ritema.pos.z + ritema.calc_bump_height < ritemb.pos.z - tolerance_z) return false;
  if(ritemb.pos.z + ritemb.calc_bump_height < ritema.pos.z - tolerance_z) return false;

  return true;
};


//--------------------------------------------------------------------------------------------
bool interact_old(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z)
{
  float dx,dy,dist;

  if(0==ritema.bump_height || 0==ritemb.bump_height) return false;

  // First check absolute value diamond
  dx = ABS(ritema.old.pos.x-ritemb.old.pos.x);
  dy = ABS(ritema.old.pos.y-ritemb.old.pos.y);
  dist = dx + dy;
  if (dist > ABS(ritema.calc_bump_size_big) + ABS(ritemb.calc_bump_size_big)) return false;
  if( dx > ABS(ritema.calc_bump_size) + ABS(ritemb.calc_bump_size) ) return false;
  if( dy > ABS(ritema.calc_bump_size) + ABS(ritemb.calc_bump_size) ) return false;

  if(ritema.old.pos.z + ritema.calc_bump_height < ritemb.old.pos.z - tolerance_z) return false;
  if(ritemb.old.pos.z + ritemb.calc_bump_height < ritema.old.pos.z - tolerance_z) return false;

  return true;
};


//--------------------------------------------------------------------------------------------
bool close_interact(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z)
{
  float dx,dy,dist;

  if(0==ritema.bump_height || 0==ritemb.bump_height) return false;

  // First check absolute value diamond
  dx = ABS(ritema.pos.x-ritemb.pos.x);
  dy = ABS(ritema.pos.y-ritemb.pos.y);
  dist = dx + dy;
  if (dist > ABS(ritema.calc_bump_size_big) || dist > ABS(ritemb.calc_bump_size_big)) return false;
  if( dx > ABS(ritema.calc_bump_size) || dx > ABS(ritemb.calc_bump_size) ) return false;
  if( dy > ABS(ritema.calc_bump_size) || dy >  ABS(ritemb.calc_bump_size) ) return false;

  if(ritema.pos.z + ritema.calc_bump_height < ritemb.pos.z - tolerance_z) return false;
  if(ritemb.pos.z + ritemb.calc_bump_height < ritema.pos.z - tolerance_z) return false;

  return true;
};

bool close_interact_old(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z)
{
  float dx,dy,dist;

  if(0==ritema.bump_height || 0==ritemb.bump_height) return false;

  // First check absolute value diamond
  dx = ABS(ritema.old.pos.x-ritemb.old.pos.x);
  dy = ABS(ritema.old.pos.y-ritemb.old.pos.y);
  dist = dx + dy;
  if (dist > ABS(ritema.calc_bump_size_big) || dist > ABS(ritemb.calc_bump_size_big)) return false;
  if( dx > ABS(ritema.calc_bump_size) || dx > ABS(ritemb.calc_bump_size) ) return false;
  if( dy > ABS(ritema.calc_bump_size) || dy >  ABS(ritemb.calc_bump_size) ) return false;

  if(ritema.old.pos.z + ritema.calc_bump_height < ritemb.old.pos.z - tolerance_z) return false;
  if(ritemb.old.pos.z + ritemb.calc_bump_height < ritema.old.pos.z - tolerance_z) return false;

  return true;
}



//--------------------------------------------------------------------------------------------
void collide_x(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  float sum  = 0;
  float diff = 0;

  float overlap = 10*ABS(rchra.pos.x - rchrb.pos.x) / (ABS(rchra.calc_bump_size) + ABS(rchrb.calc_bump_size));
  overlap = CLIP(overlap,0,1);
  float cr = MIN(rchra.bump_dampen,rchrb.bump_dampen);

  if(  (rchra.weight<0 && rchra.weight<0) || 
       (rchra.weight==0 && rchra.weight==0) || 
       (rchra.weight == rchrb.weight))
  {
    //simplest case
    sum  = (rchra.vel.x + rchrb.vel.x)*0.5f;
    diff = (rchra.vel.x - rchrb.vel.x)*0.5f;

    rchra.accumulate_vel_x( ((sum - diff*cr) - rchra.vel.x)*overlap );
    rchrb.accumulate_vel_x( ((sum + diff*cr) - rchrb.vel.x)*overlap );
  }
  else if(rchra.weight==0 || rchrb.weight<0)
  {
    sum  = (rchra.vel.x + rchrb.vel.x)*0.5f;
    diff = (rchra.vel.x - rchrb.vel.x)*0.5f;

    rchra.vel.x = (sum - diff*cr);
    rchra.accumulate_vel_x( ((sum - diff*cr) - rchra.vel.x)*overlap );
  }
  else if(rchrb.weight==0 || rchra.weight<0)
  {
    sum  = (rchra.vel.x + rchrb.vel.x)*0.5f;
    diff = (rchra.vel.x - rchrb.vel.x)*0.5f;

    rchrb.accumulate_vel_x( ((sum + diff*cr) - rchrb.vel.x)*overlap );
  }
  else
  {
    sum  = (rchra.vel.x*rchra.weight + rchrb.vel.x*rchrb.weight)*0.5f;
    diff = (rchra.vel.x*rchra.weight - rchrb.vel.x*rchrb.weight)*0.5f;

    rchra.accumulate_vel_x( ((sum - diff*cr) / rchra.weight - rchra.vel.x)*overlap );
    rchrb.accumulate_vel_x( ((sum + diff*cr) / rchrb.weight - rchrb.vel.x)*overlap );
  };
};


//--------------------------------------------------------------------------------------------
void collide_y(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  float sum  = 0;
  float diff = 0;

  float overlap = 10*ABS(rchra.pos.y - rchrb.pos.y) / (ABS(rchra.calc_bump_size) + ABS(rchrb.calc_bump_size));
  overlap = CLIP(overlap,0,1);
  float cr = MIN(rchra.bump_dampen,rchrb.bump_dampen);

  if(  (rchra.weight<0 && rchra.weight<0) || 
       (rchra.weight==0 && rchra.weight==0) || 
       (rchra.weight == rchrb.weight))
  {
    //simplest case
    sum  = (rchra.vel.y + rchrb.vel.y)*0.5f;
    diff = (rchra.vel.y - rchrb.vel.y)*0.5f;

    rchra.accumulate_vel_y( ((sum - diff*cr) - rchra.vel.y)*overlap );
    rchrb.accumulate_vel_y( ((sum + diff*cr) - rchrb.vel.y)*overlap );
  }
  else if(rchra.weight==0 || rchrb.weight<0)
  {
    sum  = (rchra.vel.y + rchrb.vel.y)*0.5f;
    diff = (rchra.vel.y - rchrb.vel.y)*0.5f;

    rchra.vel.y = (sum - diff*cr);
    rchra.accumulate_vel_y( ((sum - diff*cr) - rchra.vel.y)*overlap );
  }
  else if(rchrb.weight==0 || rchra.weight<0)
  {
    sum  = (rchra.vel.y + rchrb.vel.y)*0.5f;
    diff = (rchra.vel.y - rchrb.vel.y)*0.5f;

    rchrb.accumulate_vel_y( ((sum + diff*cr) - rchrb.vel.y)*overlap );
  }
  else
  {
    sum  = (rchra.vel.y*rchra.weight + rchrb.vel.y*rchrb.weight)*0.5f;
    diff = (rchra.vel.y*rchra.weight - rchrb.vel.y*rchrb.weight)*0.5f;

    rchra.accumulate_vel_y( ((sum - diff*cr) / rchra.weight - rchra.vel.y)*overlap );
    rchrb.accumulate_vel_y( ((sum + diff*cr) / rchrb.weight - rchrb.vel.y)*overlap );
  };
};


//--------------------------------------------------------------------------------------------
void collide_z(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  float sum  = 0;
  float diff = 0;

  float overlap = 10*ABS((rchra.pos.z + rchra.calc_bump_height) - (rchrb.pos.z + rchrb.calc_bump_height)) / (rchra.calc_bump_height*0.5 + rchrb.calc_bump_height*0.5);
  overlap = CLIP(overlap,0,1);
  float cr = MIN(rchra.bump_dampen,rchrb.bump_dampen);

  if(  (rchra.weight<0 && rchra.weight<0) || 
       (rchra.weight==0 && rchra.weight==0) || 
       (rchra.weight == rchrb.weight))
  {
    //simplest case
    sum  = (rchra.vel.z + rchrb.vel.z)*0.5f;
    diff = (rchra.vel.z - rchrb.vel.z)*0.5f;

    rchra.accumulate_vel_z( ((sum - diff*cr) - rchra.vel.z)*overlap );
    rchrb.accumulate_vel_z( ((sum + diff*cr) - rchrb.vel.z)*overlap );
  }
  else if(rchra.weight==0 || rchrb.weight<0)
  {
    sum  = (rchra.vel.z + rchrb.vel.z)*0.5f;
    diff = (rchra.vel.z - rchrb.vel.z)*0.5f;

    rchra.vel.z = (sum - diff*cr);
    rchra.accumulate_vel_z( ((sum - diff*cr) - rchra.vel.z)*overlap);
  }
  else if(rchrb.weight==0 || rchra.weight<0)
  {
    sum  = (rchra.vel.z + rchrb.vel.z)*0.5f;
    diff = (rchra.vel.z - rchrb.vel.z)*0.5f;

    rchrb.accumulate_vel_z( ((sum + diff*cr) - rchrb.vel.z)*overlap );
  }
  else
  {
    sum  = (rchra.vel.z*rchra.weight + rchrb.vel.z*rchrb.weight)*0.5f;
    diff = (rchra.vel.z*rchra.weight - rchrb.vel.z*rchrb.weight)*0.5f;

    rchra.accumulate_vel_z( ((sum - diff*cr) / rchra.weight - rchra.vel.z)*overlap );
    rchrb.accumulate_vel_z( ((sum + diff*cr) / rchrb.weight - rchrb.vel.z)*overlap );
  };
};

//--------------------------------------------------------------------------------------------
void pressure_x(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  // add in a force to push the objects appart
  float diff    = (rchra.pos.x - rchrb.pos.x);
  float overlap = ABS(diff)/(ABS(rchra.calc_bump_size) + ABS(rchrb.calc_bump_size));
  float vsum    = (rchra.vel.x + rchrb.vel.x)*0.5; 
  float vdif    = (rchra.vel.x - rchrb.vel.x)*0.5; 
  
  vdif *= GPhys.fric_h2o;

  if(rchra.weight>=0)
  {
    rchra.accumulate_vel_x( (vsum + vdif - rchra.vel.x)*overlap );
    rchra.accumulate_pos_x( diff*0.5*PLATKEEP );
  };

  if(rchrb.weight>=0)
  {
    rchrb.accumulate_vel_x( (vsum - vdif - rchrb.vel.x)*overlap );
    rchrb.accumulate_pos_x( -diff*0.5*PLATKEEP );
  };
};

//--------------------------------------------------------------------------------------------
void pressure_y(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  // add in a force to push the objects appart
  float diff    = (rchra.pos.y - rchrb.pos.y);
  float overlap = ABS(diff)/(ABS(rchra.calc_bump_size) + ABS(rchrb.calc_bump_size));
  float vsum    = (rchra.vel.y + rchrb.vel.y)*0.5; 
  float vdif    = (rchra.vel.y - rchrb.vel.y)*0.5; 
  
  vdif *= GPhys.fric_h2o;

  if(rchra.weight>=0)
  {
    rchra.accumulate_vel_y( (vsum + vdif - rchra.vel.y)*overlap );
    rchra.accumulate_pos_y( diff*0.5*PLATKEEP );
  };

  if(rchrb.weight>=0)
  {
    rchrb.accumulate_vel_y( (vsum - vdif - rchrb.vel.y)*overlap );
    rchrb.accumulate_pos_y( -diff*0.5*PLATKEEP );
  };
};

//--------------------------------------------------------------------------------------------
void pressure_z(Physics_Accumulator & rchra, Physics_Accumulator & rchrb)
{
  // add in a force to push the objects appart
  float diff    = (rchra.pos.z + rchra.calc_bump_height/2) - (rchrb.pos.z + rchrb.calc_bump_height/2);
  float overlap = ABS(diff)/(rchra.calc_bump_height/2 + rchrb.calc_bump_height/2);
  float vsum    = (rchra.vel.z + rchrb.vel.z)*0.5; 
  float vdif    = (rchra.vel.z - rchrb.vel.z)*0.5; 
  
  vdif *= GPhys.fric_h2o;

  if(rchra.weight>=0)
  {
    rchra.accumulate_vel_z( (vsum + vdif - rchra.vel.z)*overlap );
    rchra.accumulate_pos_z( diff*0.5*PLATKEEP );
  };

  if(rchrb.weight>=0)
  {
    rchrb.accumulate_vel_z( (vsum - vdif - rchrb.vel.z)*overlap );
    rchrb.accumulate_pos_z( -diff*0.5*PLATKEEP );
  };
};

//--------------------------------------------------------------------------------------------
GLMatrix CreateOrientationMatrix(float tall, float wide, Orientation & ori, bool lean_into_turn)
{
  GLMatrix mat;

  // do some pre-calculation
  float zfact = 1.0f + ori.up.z;
  float cosz  = cos_tab[ori.turn_lr>>2];
  float sinz  = sin_tab[ori.turn_lr>>2];

  if(zfact == 2.0f || zfact == 0.0f)
  {
    // generate the basic matrix
    mat.CNV(0,0) = wide*(+cosz); 
    mat.CNV(0,1) = wide*(+sinz);
    mat.CNV(0,2) = 0, 
    mat.CNV(0,3) = 0;

    mat.CNV(1,0) = wide*(-sinz); 
    mat.CNV(1,1) = wide*(+cosz);
    mat.CNV(1,2) = 0; 
    mat.CNV(1,3) = 0;

    mat.CNV(2,0) = 0;
    mat.CNV(2,1) = 0;
    mat.CNV(2,2) = tall;
    mat.CNV(2,3) = 0;

    mat.CNV(3,0) =  ori.pos.x;
    mat.CNV(3,1) =  ori.pos.y;
    mat.CNV(3,2) =  ori.pos.z;
    mat.CNV(3,3) =  1;
  }
  else
  {
    vec3_t up2(-(ori.up.x*cosz - ori.up.y*sinz), -(ori.up.x*sinz + ori.up.y*cosz),  ori.up.z);

    // generate the basic matrix
    mat.CNV(0,0) =-wide*(+cosz + ori.up.x*up2.x/zfact); 
    mat.CNV(0,1) =-wide*(-sinz + ori.up.y*up2.x/zfact);
    mat.CNV(0,2) =-wide*up2.x;
    mat.CNV(0,3) = 0;

    mat.CNV(1,0) = wide*(+sinz + ori.up.x*up2.y/zfact); 
    mat.CNV(1,1) = wide*(+cosz + ori.up.y*up2.y/zfact);
    mat.CNV(1,2) = wide*up2.y; 
    mat.CNV(1,3) = 0;

    mat.CNV(2,0) = tall*ori.up.x;
    mat.CNV(2,1) = tall*ori.up.y;
    mat.CNV(2,2) = tall*ori.up.z;
    mat.CNV(2,3) = 0;

    mat.CNV(3,0) =  ori.pos.x;
    mat.CNV(3,1) =  ori.pos.y;
    mat.CNV(3,2) =  ori.pos.z;
    mat.CNV(3,3) =  1;
  }

  return mat;
}

