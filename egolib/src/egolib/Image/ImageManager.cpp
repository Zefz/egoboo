//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************


#include "egolib/Image/ImageManager.hpp"
#include "egolib/egoboo_setup.h"
#include "egolib/log.h"
#include "egolib/Image/ImageLoader_SDL.hpp"
#include "egolib/Image/ImageLoader_SDL_image.hpp"

ImageManager *ImageManager::_singleton = nullptr;

ImageManager::ImageManager()
{
    _withSDL_image = egoboo_config_t::get().debug_sdlImage_enable.getValue();
    if (_withSDL_image)
    {
        log_info("initializing SDL_image imaging version %d.%d.%d ...", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL);
        int flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP;
        if (IMG_Init(flags) & flags != flags)
        {
            log_warning(" ... failure\n");
            _withSDL_image = false;
        }
        else
        {
            try
            {
                using namespace std;
                // Define extra supported file types with SDL_image.
                // These should probably be ordered so that the types that support transparency are first.
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".png"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tif"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tiff"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".gif"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".pcx"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".ppm"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".jpg"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".jpeg"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".xpm"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".pnm"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".lbm"))));
                // Loading TGA images using the standard method does not work according to SDL_Image documentation.
            #if 0
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tga"))));
            #endif
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".bmp"))));
                _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".BMP"))));
            }
            catch (std::exception& ex)
            {
                IMG_Quit();
                log_warning(" failure");
                _withSDL_image = false;
            }
            log_info(" success");
            return;
        }
    }
    else
    {
        log_info("SDL_image imaging disable by %s = \"false\" in `setup.txt` - only support for .bmp files\n",
                 egoboo_config_t::get().debug_sdlImage_enable.getName().c_str());
    }
    log_info("initializing standard SDL imaging ...");
    try
    {
        using namespace std;
        // These typed are natively supported with SDL.
        // Place them *after* the SDL_image types, so that if both are present,
        // the other types will be preferred over ".bmp".
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL>(new ImageLoader_SDL(".bmp"))));
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL>(new ImageLoader_SDL(".BMP"))));
    }
    catch (std::exception& ex)
    {
        log_warning(" failure");
        throw ex;
    }
    log_info(" success");
}

ImageManager::~ImageManager()
{
    if (_withSDL_image)
    {
        IMG_Quit();
    }
}

void ImageManager::initialize()
{
    if (!_singleton)
    {
        _singleton = new ImageManager();
    }
}

void ImageManager::uninitialize()
{
    if (_singleton)
    {
        delete _singleton;
        _singleton = nullptr;
    }
}

ImageManager& ImageManager::get()
{
    if (!_singleton)
    {
        throw std::logic_error("image manager not initialized");
    }
    return *_singleton;
}