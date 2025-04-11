import React from 'react';
import { FaRegBell } from 'react-icons/fa';
import './Header.css';

const Header = () => {
  return (
    <header className="header">
      <div className="header-container">
        <div className="header-content">
          {/* Logo and Title */}
          <div className="header-left">
            <span className="header-logo">🦉</span>
            <div className="header-title">
              <h1>Owl's Inc <span className="title-separator">×</span> ASU Dashboard</h1>
            </div>
          </div>

          {/* Right side items */}
          <div className="header-right">
            <button className="notification-button">
              <FaRegBell />
            </button>

            <div className="user-profile">
              <span className="user-name">Admin</span>
              <div className="user-avatar">
                <span>A</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </header>
  );
};

export default Header; 