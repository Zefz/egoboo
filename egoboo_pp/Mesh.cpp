
#include "Character.h"
#include "Particle.h"
#include "MPD_file.h"
#include "egoboo.h"

Render_List GRenderlist;
Mesh GMesh;
Bump_List BumpList;

//---------------------------------------------------------------------------------------------
//int mesh_get_level(vec3_t & pos, Uint8 waterwalk)
//{
//  // ZZ> This function returns the height of a point within a mesh fan, precise
//  //     If waterwalk is nonzero and the fan is watery, then the level returned is the
//  //     level of the water.
//  int ix,iy,fan;
//  int z0, z1, z2, z3;         // Height of each fan corner
//  int zleft, zright,zdone;    // Weighted height of each side
//
//  fan = GMesh.getIndexPos(x,y);
//  ix = int(x)&0x7F;
//  iy = int(y)&0x7F;
//
//  z0 = GMesh.vrt_z[GMesh.fan_info[fan].vrtstart+0];
//  z1 = GMesh.vrt_z[GMesh.fan_info[fan].vrtstart+1];
//  z2 = GMesh.vrt_z[GMesh.fan_info[fan].vrtstart+2];
//  z3 = GMesh.vrt_z[GMesh.fan_info[fan].vrtstart+3];
//
//  zleft = (z0*(0x80-iy)+z3*iy)>> JF::MPD_bits;
//  zright = (z1*(0x80-iy)+z2*iy)>>JF::MPD_bits;
//  zdone = (zleft*(0x80-ix)+zright*ix)>>JF::MPD_bits;
//  if (waterwalk)
//  {
//    if (WaterList.level_surface>zdone && GMesh.has_flags(fan, MESHFX_WATER) && WaterList.is_water)
//    {
//      return WaterList.level_surface;
//    }
//  }
//  return zdone;
//}
//
//--------------------------------------------------------------------------------------------
//bool get_mesh_memory()
//{
//  // ZZ> This function gets a load of memory for the terrain mesh
//  GMesh.floatmemory = (float *) malloc(GMesh.maxtotalvertices*BYTESFOREACHVERTEX);
//  if (GMesh.floatmemory == NULL)
//    return false;
//
//  GMesh.vrt_x = &GMesh.floatmemory[0];
//  GMesh.vrt_y = &GMesh.floatmemory[1*GMesh.maxtotalvertices];
//  GMesh.vrt_z = &GMesh.floatmemory[2*GMesh.maxtotalvertices];
//  GMesh.vrt_a = (Uint8 *) &GMesh.floatmemory[3*GMesh.maxtotalvertices];
//  GMesh.vrt_l = &GMesh.vrt_a[GMesh.maxtotalvertices];
//  return true;
//}

//--------------------------------------------------------------------------------------------
bool Mesh::load(char *modname)
{
  char fullmodname[0x0100];

  // load the actual mesh
  {
    make_newloadname(modname, "gamedat/level.mpd", fullmodname);

    if ( NULL == JF::MPD_Manager::loadFromFile(fullmodname, this) )
      general_error(0, 0, "LOAD PROBLEMS");
    JF::MPD_Mesh::loadFanTypes("basicdat/fans.txt");
    prepareVertices();
  }

  // now do stuff that the game needs to work with the mesh
  {
    // count the 4x4 fan-blocks
    {
      m_blocksWide = m_fansWide >> (Block_bits-JF::MPD_bits);
      if(m_blocksWide<<(Block_bits-JF::MPD_bits) < m_fansWide) m_blocksWide++;

      m_blocksHigh = m_fansHigh >> (Block_bits-JF::MPD_bits);
      if(m_blocksHigh<<(Block_bits-JF::MPD_bits) < m_fansHigh) m_blocksHigh++;

      BumpList.count = m_blocksWide * m_blocksHigh;

      m_tile_ex.Allocate(m_fansWide * m_fansHigh);
      m_fan_ex.Allocate(m_fansWide * m_fansHigh);
      m_block_ex.Allocate(m_blocksWide * m_blocksHigh);
      do_bboxes();
    }

    // pre-compute the type of each of the mesh tiles
    GRenderlist.make(GMesh);

    // pre-compute the mesh normals
    {
      do_normals();
      do_normals_smooth();
    }
  }

  return true;
};

void Mesh::do_bboxes()
{
  int i,j,k;

  // Find the bboxes for each "tile"
  for(i=0; i<m_fansWide*m_fansHigh; i++)
  {
    int numVertices = getFanType(i)->numVertices;
    int firstVertex = getFan(i)->firstVertex;

    JF::MPD_Vertex * pvert = &m_vertices[firstVertex];

    // calculate the actual bbox of the fan
    AA_BBOX tmp;
    for(j=0; j<numVertices; j++, pvert++)
    {
      m_tile_ex[i].bbox.do_merge(vec3_t(pvert->x,pvert->y,pvert->z));
    };
  };

  // determine the bboxes for each fan and block
  for(i=0; i<m_fansWide*m_fansHigh; i++)
  {

    // distribute that bbox over all tiles that it covers
    int fan_x_min = int(m_tile_ex[i].bbox.minvals.x) >> JF::MPD_bits;
    int fan_y_min = int(m_tile_ex[i].bbox.minvals.y) >> JF::MPD_bits;
    int fan_x_max = int(m_tile_ex[i].bbox.maxvals.x) >> JF::MPD_bits;
    int fan_y_max = int(m_tile_ex[i].bbox.maxvals.y) >> JF::MPD_bits;

    // Use this more complicated loop, in case the tile has been moved 
    // (like the "houses" in zippy city) or otherwise modified.  Do the best we can.
    for(int ix = fan_x_min; ix<=fan_x_max; ix++)
    {
      for(int iy = fan_y_min; iy<=fan_y_max; iy++)
      {
        if(!check_bound_tile(ix,iy)) continue;

        int fan = ix + m_fansWide*iy;

        float minx = (ix+0) << JF::MPD_bits;
        float maxx = (ix+1) << JF::MPD_bits;
        float miny = (iy+0) << JF::MPD_bits;
        float maxy = (iy+1) << JF::MPD_bits;

        if(ix==fan_x_min)
        {
          minx = m_tile_ex[i].bbox.minvals.x;
        }

        if(ix==fan_x_max)
        {
          maxx = m_tile_ex[i].bbox.maxvals.x;
        }

        if(iy==fan_y_min)
        {
          miny = m_tile_ex[i].bbox.minvals.y;
        }

        if(iy==fan_y_max)
        {
          maxy = m_tile_ex[i].bbox.maxvals.y;
        }

        m_fan_ex[i].bbox.do_merge( AA_BBOX(vec3_t(minx,miny,m_tile_ex[i].bbox.minvals.z), vec3_t(maxx,maxy,m_tile_ex[i].bbox.maxvals.z)) );
      }
    }
  };

  // Then merge the fan BBoxes into block BBoxes
  for(k=0; k<m_blocksWide*m_blocksHigh; k++)
  {
    int block_y = k / m_blocksWide;
    int block_x = k % m_blocksWide;

    int fan_x = block_x << (Block_bits - JF::MPD_bits);
    int fan_y = block_y << (Block_bits - JF::MPD_bits);

    bool found = false;
    if(check_bound_tile(fan_x+0, fan_y+0))
    {
      int fan = (fan_x+0) + (fan_y+0)*m_fansWide;
      m_block_ex[k].bbox.do_merge( m_fan_ex[fan].bbox );
    };

    if(check_bound_tile(fan_x+1,fan_y+0))
    {
      int fan = (fan_x+1) + (fan_y+0)*m_fansWide;
      m_block_ex[k].bbox.do_merge( m_fan_ex[fan].bbox );
    };

    if(check_bound_tile(fan_x+0,fan_y+1))
    {
      int fan = (fan_x+0) + (fan_y+1)*m_fansWide;
      m_block_ex[k].bbox.do_merge( m_fan_ex[fan].bbox );
    };

    if(check_bound_tile(fan_x+1,fan_y+1))
    {
      int fan = (fan_x+1) + (fan_y+1)*m_fansWide;
      m_block_ex[k].bbox.do_merge( m_fan_ex[fan].bbox );
    };
  };

};

//--------------------------------------------------------------------------------------------
bool Mesh::do_simple_normal(Uint32 fan, vec3_t & normal) const
{
  if(!check_fan(fan))
  {
    normal.x = 0;
    normal.y = 0;
    normal.z = -SGN(GPhys.gravity);
    return false;
  };

  const JF::MPD_Fan * pfan = getFan(fan);
  int vert = pfan->firstVertex;
  JF::MPD_Vertex * verts = &m_vertices[vert];

  float tl = verts[0].z;
  float tr = verts[1].z;
  float br = verts[2].z;
  float bl = verts[3].z;

  float dzdx = (br-bl+tr-tl)*0.5;
  float dzdy = (br-tr+bl-tl)*0.5;

  normal.x = -dzdx;
  normal.y = -dzdy;
  normal.z = float(0x80);

  bool retval = normalize_iterative(normal);
  if( normal.z*GPhys.gravity > 0.0f) normal *= -1.0f;

  return retval;
};



//--------------------------------------------------------------------------------------------
void Mesh::do_normals()
{
  int numFans = m_fansWide * m_fansHigh;
  _cached_normals = new vec3_t[numFans];

  for(int ix=0; ix<m_fansWide; ix++)
  {
    for(int iy = 0; iy<m_fansHigh; iy++)
    {
      int fan = ix + iy*m_fansWide;
      do_simple_normal(fan, _cached_normals[fan]);
    }
  }
};

//--------------------------------------------------------------------------------------------
void Mesh::do_normals_smooth()
{
  int numFans = m_fansWide * m_fansHigh;
  _cached_normals_smooth = new vec3_t[numFans];

  for(int ix=0; ix<m_fansWide; ix++)
  {
    for(int iy = 0; iy<m_fansHigh; iy++)
    {
      vec3_t loc_normal;
      int fan_0 = ix + iy*m_fansWide;

      for(int dx=-1; dx<=1; dx++)
      {
        int fan_x = ix + dx;
        if(fan_x<0 || fan_x>=m_fansWide) continue;

        for(int dy=-1; dy<=1; dy++)
        {
          int fan_y = iy + dy;
          if(fan_y<0 || fan_y>=m_fansHigh) continue;

          int fan_1 = fan_x + fan_y*m_fansWide;

          loc_normal += _cached_normals[fan_1];
        }
      }

      normalize_iterative(loc_normal) ;
      _cached_normals_smooth[fan_0] = loc_normal;
    }
  }
};

//--------------------------------------------------------------------------------------------
bool Mesh::simple_normal(Uint32 fan, vec3_t & normal) const
{
  if(!check_fan(fan))
  {
    normal.x = 0;
    normal.y = 0;
    normal.z = -SGN(GPhys.gravity);
    return false;
  };

  normal = _cached_normals[fan];
  return true;
}

//--------------------------------------------------------------------------------------------
bool Mesh::simple_normal(vec3_t & pos, vec3_t & normal) const
{
  if(!check_bound_pos(pos.x, pos.y))
  {
    normal.x = 0;
    normal.y = 0;
    normal.z = -SGN(GPhys.gravity);
    return false;
  };

  normal = _cached_normals[getIndexPos(pos.x, pos.y)];
  return true;
}

//--------------------------------------------------------------------------------------------
bool Mesh::smoothed_normal(vec3_t & pos, vec3_t & normal) const
{
  if(!check_bound_pos(pos.x, pos.y))
  {
    normal.x = 0;
    normal.y = 0;
    normal.z = -SGN(GPhys.gravity);
    return false;
  };

  vec3_t loc_normal;

  float lerp_x = pos.x - floor(pos.x);
  float lerp_y = pos.y - floor(pos.y);

  int fan_x = int(pos.x) >> JF::MPD_bits;
  int fan_y = int(pos.y) >> JF::MPD_bits;

  int fan = (fan_x+0) + (fan_y+0)*m_fansWide;
  if(check_fan(fan))
  {
    loc_normal += _cached_normals_smooth[fan] * (1.0f-lerp_x) * (1.0f-lerp_y);
  }

  fan = (fan_x+1) + (fan_y+0)*m_fansWide;
  if(check_fan(fan))
  {
    loc_normal += _cached_normals_smooth[fan] * lerp_x * (1.0f-lerp_y);
  }

  fan = (fan_x+0) + (fan_y+1)*m_fansWide;
  if(check_fan(fan))
  {
    loc_normal += _cached_normals_smooth[fan] * (1.0f-lerp_x) * lerp_y;
  }

  fan = (fan_x+1) + (fan_y+1)*m_fansWide;
  if(check_fan(fan))
  {
    loc_normal += _cached_normals_smooth[fan] * lerp_x * lerp_y;
  }

  float len2 = dist_squared(loc_normal);
  if(len2 == 0) 
  {
    normal.x = 0;
    normal.y = 0;
    normal.z = -SGN(GPhys.gravity);
    return false;
  };

  normal = loc_normal / sqrt(len2);
  return true;
}

//--------------------------------------------------------------------------------------------
bool Mesh::smoothed_normal(Uint32 fan, vec3_t & normal) const
{
  int fan_x = fan % m_fansWide;
  int fan_y = fan / m_fansWide;

  float fx = (fan_x+ 0.5f) * float(1<<JF::MPD_bits);
  float fy = (fan_y+ 0.5f) * float(1<<JF::MPD_bits);

  return smoothed_normal( vec3_t(fx,fy,0), normal );
}


//--------------------------------------------------------------------------------------------
//int load_mesh(Mesh & msh, char *modname)
//{
//  // ZZ> This function loads the level.mpd file
//  FILE* fileread;
//  char newloadname[0x0100];
//  int itmp, cnt;
//  float ftmp;
//  int fan;
//  int numvert;
//  int x, y, vert;
//
//  make_newloadname(modname, "gamedat/level.mpd", newloadname);
//  fileread = fopen(newloadname, "rb");
//  if (fileread)
//  {
//    fread(&itmp, 4, 1, fileread);  if ((int)SDL_SwapBE32(itmp) != MAPID) return false;
//    fread(&itmp, 4, 1, fileread);  numvert = (int)SDL_SwapBE32(itmp);
//    fread(&itmp, 4, 1, fileread);  msh.size_x = (int)SDL_SwapBE32(itmp);
//    fread(&itmp, 4, 1, fileread);  msh.size_y = (int)SDL_SwapBE32(itmp);
//
//
//    msh.fan_count = msh.size_x*msh.size_y;
//    msh.edge_x = msh.size_x*0x80;
//    msh.edge_y = msh.size_y*0x80;
//    WaterList.shift = 3;
//    if (msh.size_x > 16)  WaterList.shift++;
//    if (msh.size_x > 0x20)  WaterList.shift++;
//    if (msh.size_x > 0x40)  WaterList.shift++;
//    if (msh.size_x > 0x80)  WaterList.shift++;
//    if (msh.size_x > 0x0100)  WaterList.shift++;
//
//    // Load fan data
//    fan = 0;
//    while (fan < msh.fan_count)
//    {
//      fread(&itmp, 4, 1, fileread);
//
//      msh.fan_info[fan].type = SDL_SwapBE32(itmp)>>24;
//      msh.fan_info[fan].fx   = SDL_SwapBE32(itmp)>>16;
//      msh.fan_info[fan].tile = SDL_SwapBE32(itmp);
//
//      fan++;
//    }
//    // Load fan data
//    fan = 0;
//    while (fan < msh.fan_count)
//    {
//      fread(&itmp, 1, 1, fileread);
//
//      msh.fan_info[fan].twist = SDL_SwapBE32(itmp);
//
//      fan++;
//    }
//
//    // Load vertex x data
//    cnt = 0;
//    while (cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//
//      msh.vrt_x[cnt] = SDL_SwapLEFloat(ftmp);
//
//      cnt++;
//    }
//    // Load vertex y data
//    cnt = 0;
//    while (cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//
//      msh.vrt_y[cnt] = SDL_SwapLEFloat(ftmp);
//
//      cnt++;
//    }
//    // Load vertex z data
//    cnt = 0;
//    while (cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//
//      msh.vrt_z[cnt] = (SDL_SwapLEFloat(ftmp))/16.0;    // Cartman uses 4 bit fixed point for Z
//
//      cnt++;
//    }
//
//    // GS - set to if(1) to disable lighting!!!!
//    if (0)   //(shading == D3DSHADE_FLAT && !rts_mode)
//    {
//      // Assume fullbright
//      cnt = 0;
//      while (cnt < numvert)
//      {
//        msh.vrt_a[cnt] = 0xFF;
//        msh.vrt_l[cnt] = 0xFF;
//        cnt++;
//      }
//    }
//    else
//    {
//      // Load vertex a data
//      cnt = 0;
//      while (cnt < numvert)
//      {
//        fread(&itmp, 1, 1, fileread);
//        msh.vrt_a[cnt] = SDL_SwapBE32(itmp);
//        msh.vrt_l[cnt] = 0;
//        cnt++;
//      }
//    }
//    fclose(fileread);
//
//    make_fanstart();
//
//    vert = 0;
//    y = 0;
//    while (y < msh.size_y)
//    {
//      x = 0;
//      while (x < msh.size_x)
//      {
//        fan = GMesh.getIndexTile(x,y);
//        msh.fan_info[fan].vrtstart = vert;
//        vert+=msh.commandnumvertices[msh.fan_info[fan].type];
//        x++;
//      }
//      y++;
//    }
//
//    return true;
//  }
//  return false;
//}
//
//
//


//grfx.c
//--------------------------------------------------------------------------------------------
//void load_mesh_fans()
//{
//  // ZZ> This function loads fan types for the terrain
//  int cnt, entry;
//  int numfantype, fantype, bigfantype, vertices;
//  int numcommand, command, commandsize;
//  int itmp;
//  float ftmp;
//  FILE* fileread;
//  float offx, offy;
//
//  // Initialize all mesh types to 0
//  entry = 0;
//  while (entry < MAXMESHTYPE)
//  {
//    GMesh.commandnumvertices[entry] = 0;
//    GMesh.commands[entry] = 0;
//    entry++;
//  }
//
//  // Open the file and go to it
//  fileread = fopen("basicdat/fans.txt", "r");
//  if (fileread)
//  {
//    goto_colon(fileread);
//    fscanf(fileread, "%d", &numfantype);
//    fantype = 0;
//    bigfantype = MAXMESHTYPE/2; // Duplicate for 64x64 tiles
//    while (fantype < numfantype)
//    {
//      goto_colon(fileread);
//      fscanf(fileread, "%d", &vertices);
//      GMesh.commandnumvertices[fantype] = vertices;
//      GMesh.commandnumvertices[bigfantype] = vertices;  // Dupe
//      cnt = 0;
//      while (cnt < vertices)
//      {
//        goto_colon(fileread);
//        fscanf(fileread, "%d", &itmp);
//        goto_colon(fileread);
//        fscanf(fileread, "%f", &ftmp);
//        GMesh.commandu[fantype][cnt] = ftmp;
//        GMesh.commandu[bigfantype][cnt] = ftmp;  // Dupe
//        goto_colon(fileread);
//        fscanf(fileread, "%f", &ftmp);
//        GMesh.commandv[fantype][cnt] = ftmp;
//        GMesh.commandv[bigfantype][cnt] = ftmp;  // Dupe
//        cnt++;
//      }
//
//      goto_colon(fileread);
//      fscanf(fileread, "%d", &numcommand);
//      GMesh.commands[fantype] = numcommand;
//      GMesh.commands[bigfantype] = numcommand;  // Dupe
//      entry = 0;
//      command = 0;
//      while (command < numcommand)
//      {
//        goto_colon(fileread);
//        fscanf(fileread, "%d", &commandsize);
//        GMesh.commandsize[fantype][command] = commandsize;
//        GMesh.commandsize[bigfantype][command] = commandsize;  // Dupe
//        cnt = 0;
//        while (cnt < commandsize)
//        {
//          goto_colon(fileread);
//          fscanf(fileread, "%d", &itmp);
//          GMesh.commandvrt[fantype][entry] = itmp;
//          GMesh.commandvrt[bigfantype][entry] = itmp;  // Dupe
//          entry++;
//          cnt++;
//        }
//        command++;
//      }
//      fantype++;
//      bigfantype++;  // Dupe
//    }
//    fclose(fileread);
//  }
//
//  // Correct all of them silly texture positions for seamless tiling
//  entry = 0;
//  while (entry < MAXMESHTYPE/2)
//  {
//    cnt = 0;
//    while (cnt < GMesh.commandnumvertices[entry])
//    {
//      //            GMesh.commandu[entry][cnt] = ((.5/0x20)+(GMesh.commandu[entry][cnt]*0x1F/0x20))/8;
//      //            GMesh.commandv[entry][cnt] = ((.5/0x20)+(GMesh.commandv[entry][cnt]*0x1F/0x20))/8;
//      GMesh.commandu[entry][cnt] = ((.6/0x20)+(GMesh.commandu[entry][cnt]*30.8/0x20))/8;
//      GMesh.commandv[entry][cnt] = ((.6/0x20)+(GMesh.commandv[entry][cnt]*30.8/0x20))/8;
//      cnt++;
//    }
//    entry++;
//  }
//  // Do for big tiles too
//  while (entry < MAXMESHTYPE)
//  {
//    cnt = 0;
//    while (cnt < GMesh.commandnumvertices[entry])
//    {
//      //            GMesh.commandu[entry][cnt] = ((.5/0x40)+(GMesh.commandu[entry][cnt]*0x3F/0x40))/4;
//      //            GMesh.commandv[entry][cnt] = ((.5/0x40)+(GMesh.commandv[entry][cnt]*0x3F/0x40))/4;
//      GMesh.commandu[entry][cnt] = ((.6/0x40)+(GMesh.commandu[entry][cnt]*62.8/0x40))/4;
//      GMesh.commandv[entry][cnt] = ((.6/0x40)+(GMesh.commandv[entry][cnt]*62.8/0x40))/4;
//      cnt++;
//    }
//    entry++;
//  }
//
//  // Make tile texture offsets
//  entry = 0;
//  while (entry < MAXTILETYPE)
//  {
//    offx = (entry&7)/8.0;
//    offy = (entry>>3)/8.0;
//    GMesh.tile_off_u[entry] = offx;
//    GMesh.tile_off_v[entry] = offy;
//    entry++;
//  }
//}
//
//--------------------------------------------------------------------------------------------
//void make_fanstart()
//{
//  // ZZ> This function builds a look up table to ease calculating the
//  //     fan number given an x,y pair
//  int cnt;
//
//  cnt = 0;
//  while (cnt < GMesh.fansHigh())
//  {
//    GMesh.fan_start[cnt] = GMesh.fansWide()*cnt;
//    cnt++;
//  }
//
//  cnt = 0;
//  while (cnt < (GMesh.fansHigh()>>2))
//  {
//    //GMesh.blockstart[cnt] = (GMesh.fansWide()>>2)*cnt;
//    cnt++;
//  }
//}
//
//--------------------------------------------------------------------------------------------
//void twist_to_normal(Uint8 twist, float * nrm_vec) 
//{
//  int iy = twist>>4;
//  int ix = twist&15;
//
//  iy -= 7;  // -7 to 8
//  ix -= 7;  // -7 to 8
//
//  if (ABS(iy) >= 7) iy <<= 1;
//  if (ABS(ix) >= 7) ix <<= 1;
//
//  float nrm = 0.072168783648703220563643597562745; // == 1/sqrt(3)/8;
//  nrm_vec[0] = ix*nrm;
//  nrm_vec[1] = iy*nrm;
//  nrm_vec[2] = sqrt(1 - nrm_vec[0]*nrm_vec[0] - nrm_vec[1]*nrm_vec[1]);
//};

//--------------------------------------------------------------------------------------------
//void make_twist()
//{
//  // ZZ> This function precomputes surface normals and steep hill acceleration for
//  //     the mesh
//  int cnt;
//  int x, y;
//  float xslide, yslide;
//
//  cnt = 0;
//  while (cnt < 0x0100)
//  {
//    y = cnt>>4;
//    x = cnt&15;
//    y = y-7;  // -7 to 8
//    x = x-7;  // -7 to 8
//    map_twist_ud[cnt] = 0x8000+y*SLOPE;
//    map_twist_lr[cnt] = 0x8000+x*SLOPE;
//    if (ABS(y) >=7) y=y<<1;
//    if (ABS(x) >=7) x=x<<1;
//    xslide = x*SLIDE;
//    yslide = y*SLIDE;
//    if (xslide < 0)
//    {
//      xslide+=SLIDEFIX;
//      if (xslide > 0)
//        xslide=0;
//    }
//    else
//    {
//      xslide-=SLIDEFIX;
//      if (xslide < 0)
//        xslide=0;
//    }
//    if (yslide < 0)
//    {
//      yslide+=SLIDEFIX;
//      if (yslide > 0)
//        yslide=0;
//    }
//    else
//    {
//      yslide-=SLIDEFIX;
//      if (yslide < 0)
//        yslide=0;
//    }
//    vel_twist_ud[cnt] = -yslide*GPhys.fric_hill;
//    vel_twist_lr[cnt] = xslide*GPhys.fric_hill;
//    flat_twist[cnt] = false;
//    if (ABS(vel_twist_ud[cnt]) + ABS(vel_twist_lr[cnt]) < SLIDEFIX*4)
//    {
//      flat_twist[cnt] = true;
//    }
//    cnt++;
//  }
//}

//--------------------------------------------------------------------------------------------
bool mesh_is_over_water(vec3_t & pos)
{
  // This function returns true if the particle is over a water tile

  if(!WaterList.is_water) return false;

  return GMesh.has_flags(pos.x,pos.y,MESHFX_WATER);
}

//--------------------------------------------------------------------------------------------
bool Mesh::check_fanblock_block(int ix, int iy) const
{
  return ix>=0 && iy>=0 && ix<m_blocksWide && iy<m_blocksHigh;
}

//--------------------------------------------------------------------------------------------
void Mesh::getRegionFromFan(Uint32 fan, REGION & r) const
{
  int ix = fan % m_fansWide;
  int iy = fan / m_fansWide;

  r.left   = ix << JF::MPD_bits;
  r.top    = iy << JF::MPD_bits;
  r.width  = 1  << JF::MPD_bits;
  r.height = 1  << JF::MPD_bits;
};

//--------------------------------------------------------------------------------------------
void Mesh::getRegionFromFanblock(Uint32 block, REGION & r) const
{
  int ix = block % m_blocksWide;
  int iy = block / m_blocksWide;

  r.left   = ix << Block_bits;
  r.top    = iy << Block_bits;
  r.width  = 1  << Block_bits;
  r.height = 1  << Block_bits;
};

//--------------------------------------------------------------------------------------------
Uint32 Mesh::getFanblockPos(vec3_t & pos) const
{
  if(!check_bound_pos(pos.x,pos.y)) return Mesh::INVALID_INDEX;

  int ix = int(pos.x) >> Mesh::Block_bits, iy=int(pos.y) >> Mesh::Block_bits;
  return ix + iy*m_blocksWide;
}

//--------------------------------------------------------------------------------------------
Uint32 Mesh::getFanblockBlock(int blockx, int blocky) const
{
  if(!check_fanblock_block(blockx,blocky)) return Mesh::INVALID_INDEX;

  return blockx + blocky*m_blocksWide;
}

//--------------------------------------------------------------------------------------------
Uint32 Mesh::get_fanblock_pos(vec3_t & pos) const
{
  int ix = int(pos.x) >> Mesh::Block_bits, iy=int(pos.y) >> Mesh::Block_bits;
  return ix + iy*m_blocksWide;
}

//--------------------------------------------------------------------------------------------
Uint32 Mesh::get_fanblock_block(int ix, int iy) const
{
  return ix + iy*m_blocksWide;
}

//--------------------------------------------------------------------------------------------
void Render_List::reset(Mesh & mlist)
{
  // Clear old render lists
  size_t fan, cnt = 0;
  while(cnt < all_count)
  {
    fan = _list[cnt++].all;
    //mlist.fan_info[fan].inrenderlist = false;
  }
  all_count = ref_count = sha_count = drf_count =
  ani_count = wat_count = wal_count = imp_count =
  dam_count = slp_count = 0;
};

//--------------------------------------------------------------------------------------------
void Render_List::make(Mesh & mlist)
{
  // ZZ> This function figures out which mesh fans to draw
  int fan, fan_cnt;

  reset(mlist);

  fan_cnt      =  mlist.fansHigh()     *  mlist.fansWide();

  // Add EVERY tile, but sort them
  for(fan=0; fan<fan_cnt; fan++)
  {
    _list[all_count++].all = fan;
    //mlist.fan_info[fan].inrenderlist = true;

    if( mlist.has_flags(fan, MESHFX_REF) )
    {
      _list[sha_count++].ref = fan;
    }

    if( mlist.has_flags(fan, MESHFX_SHA) )
    {
      _list[ref_count++].ref = fan;
    }

    if( mlist.has_flags(fan, MESHFX_DRAWREF) )
    {
      _list[drf_count++].drf = fan;
    }

    if( mlist.has_flags(fan, MESHFX_ANIM) )
    {
      _list[ani_count++].ani = fan;
    }

    if( mlist.has_flags(fan, MESHFX_WATER) )
    {
      _list[wat_count++].wat = fan;
    }

    if( mlist.has_flags(fan, MESHFX_WALL) )
    {
      _list[wal_count++].wal = fan;
    }

    if( mlist.has_flags(fan, MESHFX_IMPASS) )
    {
      _list[imp_count++].imp = fan;
    }

    if( mlist.has_flags(fan, MESHFX_DAMAGE) )
    {
      _list[dam_count++].dam = fan;
    }

    if( mlist.has_flags(fan, MESHFX_SLIPPY) )
    {
      _list[slp_count++].slp = fan;
    }
  };

}



//--------------------------------------------------------------------------------------------
//void Render_List::reset(JF::MPD_Mesh * pmesh)
//{
//  // Clear old render lists
//  size_t fan, cnt = 0;
//  while(cnt < all_count)
//  {
//    fan = _list[cnt++].all;
//    //GMesh.fan_info[fan].inrenderlist = false;
//  }
//  all_count = ref_count = sha_count = drf_count =
//  ani_count = wat_count = wal_count = imp_count =
//  dam_count = slp_count = 0;
//};
////--------------------------------------------------------------------------------------------
//void Render_List::make(JF::MPD_Mesh * pmesh)
//{
//  // ZZ> This function figures out which mesh fans to draw
//  int fan;
//
//  reset(pmesh);
//
//  // Add EVERY tile, but sort them
//  for(fan=0; fan<pmesh->numFans(); fan++)
//  {
//    _list[all_count++].all = fan;
//    //GMesh.fan_info[fan].inrenderlist = true;
//
//    const JF::MPD_Fan * pfan = pmesh->getFan(fan);
//
//    if(pfan->flags&MESHFX_REF)
//    {
//      _list[sha_count++].ref = fan;
//    }
//
//    if(pfan->flags&MESHFX_SHA)
//    {
//      _list[ref_count++].ref = fan;
//    }
//
//    if(pfan->flags&MESHFX_DRAWREF)
//    {
//      _list[drf_count++].drf = fan;
//    }
//
//    if(pfan->flags&MESHFX_ANIM)
//    {
//      _list[ani_count++].ani = fan;
//    }
//
//    if(pfan->flags&MESHFX_WATER)
//    {
//      _list[wat_count++].wat = fan;
//    }
//
//    if(pfan->flags&MESHFX_WALL)
//    {
//      _list[wal_count++].wal = fan;
//    }
//
//    if(pfan->flags&MESHFX_IMPASS)
//    {
//      _list[imp_count++].imp = fan;
//    }
//
//    if(pfan->flags&MESHFX_DAMAGE)
//    {
//      _list[dam_count++].dam = fan;
//    }
//
//    if(pfan->flags&MESHFX_SLIPPY)
//    {
//      _list[slp_count++].slp = fan;
//    }
//  };
//
//}

//---------------------------------------------------------------------------------------------
void Bump_List::clear()
{
  Uint32 fanblock = 0;
  while (fanblock < count)
  {
    _list[fanblock].chr       = Character_List::INVALID;
    _list[fanblock].chr_count = 0;
    _list[fanblock].prt       = Particle_List::INVALID;
    _list[fanblock].prt_count = 0;
    fanblock++;
  }
};