import React from 'react';

const Skeleton = ({ width, height, borderRadius = 'var(--radius-md)', className = '' }) => {
  return (
    <div 
      className={`skeleton-pulse ${className}`}
      style={{ 
        width: width || '100%', 
        height: height || '100%', 
        borderRadius 
      }} 
    />
  );
};

export default Skeleton;
