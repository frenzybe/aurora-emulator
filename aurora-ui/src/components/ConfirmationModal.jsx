import React from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { AlertCircle } from 'lucide-react';

const ConfirmationModal = ({ isOpen, title, message, onConfirm, onCancel }) => {
  return (
    <AnimatePresence>
      {isOpen && (
        <div className="modal-overlay">
          <motion.div 
            initial={{ scale: 0.9, opacity: 0 }}
            animate={{ scale: 1, opacity: 1 }}
            exit={{ scale: 0.9, opacity: 0 }}
            className="modal-content"
          >
            <div className="flex-col align-center">
              <div className="mb-m" style={{ color: '#ff4d4d' }}>
                <AlertCircle size={48} />
              </div>
              <h2 className="text-md mb-s">{title}</h2>
              <p className="text-xs text-dim mb-l">{message}</p>
              
              <div className="flex-row gap-m w-full">
                <button 
                  className="btn-secondary flex-1" 
                  onClick={onCancel}
                >
                  Cancel
                </button>
                <button 
                  className="btn-primary flex-1" 
                  style={{ background: '#ff4d4d', color: '#fff' }}
                  onClick={onConfirm}
                >
                  Delete
                </button>
              </div>
            </div>
          </motion.div>
        </div>
      )}
    </AnimatePresence>
  );
};

export default ConfirmationModal;
