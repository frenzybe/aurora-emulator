export const getPlatform = (path) => {
  if (!path) return { name: 'UNKNOWN', shortName: '?', ext: '', raId: 0 };
  const ext = path.split('.').pop().toLowerCase();
  
  switch (ext) {
    case 'nes':
      return { name: 'Nintendo Entertainment System', shortName: 'NES', ext: 'nes', raId: 7 };
    case 'sfc':
    case 'smc':
      return { name: 'Super Nintendo', shortName: 'SNES', ext: 'snes', raId: 3 };
    case 'gb':
      return { name: 'Game Boy', shortName: 'GB', ext: 'gb', raId: 4 };
    case 'gbc':
      return { name: 'Game Boy Color', shortName: 'GBC', ext: 'gbc', raId: 6 };
    case 'gba':
      return { name: 'Game Boy Advance', shortName: 'GBA', ext: 'gba', raId: 5 };
    case 'sms':
      return { name: 'Master System', shortName: 'SMS', ext: 'sms', raId: 8 };
    case 'gg':
      return { name: 'Game Gear', shortName: 'GG', ext: 'gg', raId: 7 };
    case 'pce':
      return { name: 'PC Engine', shortName: 'PCE', ext: 'pce', raId: 9 };
    case 'md':
    case 'bin':
    case 'gen':
      return { name: 'Mega Drive', shortName: 'GEN', ext: 'gen', raId: 1 };
    case 'ps1':
    case 'psx':
    case 'cue':
    case 'chd':
      return { name: 'PlayStation', shortName: 'PSX', ext: 'psx', raId: 12 };
    case 'n64':
    case 'v64':
    case 'z64':
      return { name: 'Nintendo 64', shortName: 'N64', ext: 'n64', raId: 11 };
    case 'nds':
      return { name: 'Nintendo DS', shortName: 'NDS', ext: 'nds', raId: 18 };
    case 'psp':
    case 'iso':
      // Smart detection for ISO files based on filename hints
      const upperPath = path.toUpperCase();
      if (upperPath.includes('PS2') || upperPath.includes('PLAYSTATION 2') || upperPath.includes('PLAYSTATION2')) {
        return { name: 'PlayStation 2', shortName: 'PS2', ext: 'ps2', raId: 28 };
      }
      if (upperPath.includes('PS1') || upperPath.includes('PSX') || upperPath.includes('PLAYSTATION 1') || upperPath.includes('PLAYSTATION1')) {
        return { name: 'PlayStation', shortName: 'PSX', ext: 'psx', raId: 12 };
      }
      return { name: 'PlayStation Portable', shortName: 'PSP', ext: 'psp', raId: 41 };
    case 'cdi':
    case 'gdi':
      return { name: 'Dreamcast', shortName: 'DC', ext: 'cdi', raId: 21 };
    case 'gcm':
    case 'rvz':
    case 'iso_wii': // Pseudo-ext for internal use if needed
      return { name: 'GameCube / Wii', shortName: 'GC', ext: 'rvz', raId: 27 };
    case 'dos':
    case 'exe':
    case 'conf':
      return { name: 'DOS', shortName: 'DOS', ext: 'dos', raId: 0 };
    case 'ss':
    case 'saturn':
      return { name: 'Sega Saturn', shortName: 'SAT', ext: 'ss', raId: 23 };
    case '3do':
      return { name: 'Panasonic 3DO', shortName: '3DO', ext: '3do', raId: 30 };
    case 'a26':
    case 'bin_atari':
      return { name: 'Atari 2600', shortName: 'A26', ext: 'a26', raId: 25 };
    case 'a78':
      return { name: 'Atari 7800', shortName: 'A78', ext: 'a78', raId: 26 };
    case 'lnx':
      return { name: 'Atari Lynx', shortName: 'LYNX', ext: 'lnx', raId: 24 };
    case 'ps2':
    case 'iso_ps2':
      return { name: 'PlayStation 2', shortName: 'PS2', ext: 'ps2', raId: 28 };
    case '3ds':
    case 'cia':
      return { name: 'Nintendo 3DS', shortName: '3DS', ext: '3ds', raId: 0 };
    case 'msx':
    case 'msx1':
    case 'msx2':
    case 'dsk':
      return { name: 'MSX / MSX2', shortName: 'MSX', ext: 'msx', raId: 19 };
    case 'adf':
      return { name: 'Commodore Amiga', shortName: 'AMI', ext: 'adf', raId: 49 };
    case 'scummvm':
      return { name: 'ScummVM', shortName: 'SVM', ext: 'scummvm', raId: 0 };
    case 'ws':
    case 'wsc':
      return { name: 'WonderSwan Color', shortName: 'WS', ext: 'wsc', raId: 20 };
    case 'ngp':
    case 'ngc':
      return { name: 'Neo Geo Pocket', shortName: 'NGP', ext: 'ngp', raId: 22 };
    case 'vb':
      return { name: 'Virtual Boy', shortName: 'VB', ext: 'vb', raId: 17 };
    case 'o2':
    case 'bin_o2':
      return { name: 'Odyssey 2', shortName: 'O2', ext: 'o2', raId: 48 };
    case 'zip':
    case '7z':
      // ZIP is ambiguous, could be Arcade or Neo Geo
      return { name: 'Arcade / Multi', shortName: 'ARC', ext: 'zip', raId: 0 };
    default:
      return { name: 'Retro Console', shortName: 'ROM', ext: 'gen', raId: 1 };
  }
};
