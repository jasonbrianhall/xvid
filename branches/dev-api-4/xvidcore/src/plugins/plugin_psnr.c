#include "../xvid.h"
#include "../image/image.h"


int xvid_plugin_psnr(void * handle, int opt, void * param1, void * param2)
{
    switch(opt)
    {
    case XVID_PLG_INFO :
        {
        xvid_plg_info_t * info = (xvid_plg_info_t*)param1;
        info->flags = XVID_REQORIGINAL;
        return 0;
        }

    case XVID_PLG_CREATE :
    case XVID_PLG_DESTROY :
    case XVID_PLG_BEFORE :
       return 0;

    case XVID_PLG_AFTER :
       {
       xvid_plg_data_t * data = (xvid_plg_data_t*)param1;
          
       long sse_y = plane_sse(data->original.plane[0], data->current.plane[0], 
                data->current.stride[0], data->width, data->height);

       long sse_u = plane_sse(data->original.plane[1], data->current.plane[1], 
                data->current.stride[1], data->width/2, data->height/2);
       
       long sse_v = plane_sse(data->original.plane[2], data->current.plane[2], 
                data->current.stride[2], data->width/2, data->height/2);

       printf("y_psnr=%2.2f u_psnr=%2.2f v_psnr=%2.2f\n", 
           sse_to_PSNR(sse_y, data->width*data->height),
           sse_to_PSNR(sse_u, data->width*data->height/4),
           sse_to_PSNR(sse_v, data->width*data->height/4));

       {
           IMAGE img;
           char tmp[100];
           img.y = data->original.plane[0];
           img.u = data->original.plane[1];
           img.v = data->original.plane[2];
           sprintf(tmp, "ori-%03i.pgm", data->frame_num);
           image_dump_yuvpgm(&img, data->original.stride[0], data->width, data->height, tmp);

           img.y = data->current.plane[0];
           img.u = data->current.plane[1];
           img.v = data->current.plane[2];
           sprintf(tmp, "enc-%03i.pgm", data->frame_num);
           image_dump_yuvpgm(&img, data->reference.stride[0], data->width, data->height, tmp);
       }

       return 0;
       }
    }

    return XVID_ERR_FAIL;
}
