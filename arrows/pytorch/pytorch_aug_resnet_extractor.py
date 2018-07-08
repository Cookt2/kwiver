from __future__ import print_function
from __future__ import division
from __future__ import absolute_import

import torch
import torch.utils.data as data
import torch.nn as nn

from torchvision import models, transforms, datasets
from torch.autograd import Variable

from PIL import Image as pilImage

import math
import random

import numpy as np

from vital.types import BoundingBox


def augment_region( input_image, cx, cy, csize, outsize, rot, tflux=6, sflux=0.3, iflux=0.2 ):

    if 'cv2' not in sys.modules:
        import cv2

    rt2 = math.sqrt( 2.0 )
    csize = csize * ( 1.0 + random.uniform( -sflux, sflux ) )
    boxsize = int( csize * rt2 )
    halfbox = int( boxsize / 2 )

    ul = np.array( [random.randint(-tflux,tflux)+cx-halfbox, random.randint(-tflux,tflux)+cy-halfbox] ).astype(int)
    lr = np.array( [ul[0]+boxsize, ul[1]+boxsize] ).astype(int)

    crop = np.asarray(input_image.crop((ul[0],ul[1],lr[0],lr[1])))

    halfboxsize = boxsize / 2.0
    A = cv2.getRotationMatrix2D( (halfboxsize, halfboxsize), rot, 1.0 )
    rcrop = cv2.warpAffine( crop, A, (boxsize,boxsize))

    halfcsize = csize / 2.0
    ul = np.array([int(halfboxsize - halfcsize), int(halfboxsize - halfcsize)])
    lr = np.array([int(halfboxsize + halfcsize), int(halfboxsize + halfcsize)])
    rcrop = rcrop[ul[1]:lr[1],ul[0]:lr[0]]

    iaug = ( rcrop + ( 255 * random.uniform( -iflux, iflux ) ) ) * ( 1.0 + random.uniform( -iflux, iflux ) )

    iaug[ iaug > 255 ] = 255
    iaug[ iaug < 0 ] = 0

    crop = cv2.resize(iaug,tuple([outsize,outsize]))
    return crop


class aug_resnet_data_loader(data.Dataset):
    def __init__(self, bbox_list, transform, frame_img, in_size, rot_shifts):
        self._frame_img = pilImage.new( "RGB", frame_img.size )
        self._frame_img.paste( frame_img )
        self._transform = transform
        self._bbox_list = bbox_list
        self._in_size = in_size
        self._rot_shifts = rot_shifts

    def __getitem__(self, index):
        bid = int(math.floor(index / self._rot_shifts))
        bb = self._bbox_list[bid].bounding_box()
        rot = float( 360.0 * ( index % self._rot_shifts ) ) / self._rot_shifts - 180.0

        # unwrap
        min_x = float( bb.min_x() ) 
        min_y = float( bb.min_y() )
        max_x = float( bb.max_x() )
        max_y = float( bb.max_y() )

        c_x = ( min_x + max_x ) / 2
        c_y = ( min_y + max_y ) / 2

        diameter = 1.2 * max( max_x - min_x, max_y - min_y )

        crop = augment_region( self._frame_img, c_x, c_y, diameter, self._in_size, rot )

        # DEBUG
        #print( index )
        #print( bid )
        #print( rot )
        #print( crop.shape )
        #outfile2 = "augmented_" + str( index ) + ".png"
        #cv2.imwrite( outfile2, crop )
        # DEBUG

        if self._transform is not None:
            im = self._transform(pilImage.fromarray(crop.astype('uint8'), 'RGB'))
        else:
            im = pilImage.fromarray(crop)

        return im

    def __len__(self):
        return self._bbox_list.size() * self._rot_shifts
    

class pytorch_aug_resnet_extractor(object):
    """
    Obtain the appearance features from a trained pytorch resnet50
    model
    """

    def __init__(self, resnet_model_path, img_size, batch_size, GPU_list=None, rotational_shifts=360, resize_factor=0.2, int_shift_factor=0.2):

        if GPU_list is None:
            GPU_list = [x for x in range(torch.cuda.device_count())]
            target_GPU = 0 # I assume this is just hardcoding in using the first GPU
        else:
            target_GPU = GPU_list[0]

        # load the resnet50 model. Maybe this shouldn't be hardcoded?
        self._resnet_model = models.resnet50().cuda(device=target_GPU)
   
        weights = torch.load( resnet_model_path )
        self._resnet_model.load_state_dict( weights )
        self._resnet_model = nn.Sequential(*list(self._resnet_model.children())[:-1])

        self._resnet_model.train( False ) # is this the same as eval() ?
        self._resnet_model.cuda() # move the model to the GPU
 
        self._transform = transforms.Compose([
            transforms.Scale(img_size),
            transforms.ToTensor(),
            transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
        ])

        self._img_size = img_size
        self._b_size = batch_size
        self._rotational_shifts = rotational_shifts
        self._resize_factor = resize_factor
        self._int_shift_factor = int_shift_factor

    @property
    def frame(self):
        return self._frame

    @frame.setter
    def frame(self, val):
        self._frame = val

    def __call__(self, bbox_list):
        return self._obtain_feature(bbox_list)

    def _obtain_feature(self, bbox_list):

        kwargs = {'num_workers': 8, 'pin_memory': True}
        bbox_loader_class = aug_resnet_data_loader(bbox_list, self._transform, self._frame, self._img_size, self._rotational_shifts) 
        bbox_loader = torch.utils.data.DataLoader(bbox_loader_class, batch_size=self._b_size, shuffle=False, **kwargs)

        for idx, imgs in enumerate(bbox_loader):
            v_imgs = Variable(imgs).cuda()
            output = self._resnet_model(v_imgs)

            if idx == 0:
                app_features = output.data
            else:
                app_features = torch.cat((app_features, output.data), dim=0)

        return app_features.cpu()