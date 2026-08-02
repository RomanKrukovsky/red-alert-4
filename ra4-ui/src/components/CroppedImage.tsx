import React from 'react';
import classNames from 'classnames';

interface CroppedImageProps extends React.HTMLAttributes<HTMLDivElement> {
  src: string;
  width: number | string;
  height: number | string;
  x: number;
  y: number;
  originalWidth?: number;
  originalHeight?: number;
  zoom?: number; // to scale the extracted part
}

/**
 * Extracts a portion of a larger image (like a screenshot) using background-position.
 * Assumes the original image is 1920x1080 by default.
 */
export const CroppedImage: React.FC<CroppedImageProps> = ({
  src,
  width,
  height,
  x,
  y,
  originalWidth = 1920,
  originalHeight = 1080,
  zoom = 1,
  className,
  style,
  ...props
}) => {
  return (
    <div
      className={classNames('ra4-cropped-image', className)}
      style={{
        width,
        height,
        backgroundImage: `url(${src})`,
        backgroundPosition: `-${x * zoom}px -${y * zoom}px`,
        backgroundSize: `${originalWidth * zoom}px ${originalHeight * zoom}px`,
        backgroundRepeat: 'no-repeat',
        ...style
      }}
      {...props}
    />
  );
};
