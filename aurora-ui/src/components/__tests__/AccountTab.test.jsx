import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi } from 'vitest'
import React from 'react'
import AccountTab from '../AccountTab'

// Mock dependencies
vi.mock('@tauri-apps/api/core', () => ({
  invoke: vi.fn(),
}))

vi.mock('@tauri-apps/plugin-shell', () => ({
  open: vi.fn(),
}))

vi.mock('../hooks/useProfile', () => ({
  clearProfileCache: vi.fn(),
}))

vi.mock('../hooks/useAchievements', () => ({
  clearAchievementCache: vi.fn(),
}))

vi.mock('lucide-react', () => ({
  User: () => <div data-testid="user-icon" />,
  ExternalLink: () => <div data-testid="external-icon" />,
  Trophy: () => <div data-testid="trophy-icon" />,
  Star: () => <div data-testid="star-icon" />,
  Shield: () => <div data-testid="shield-icon" />,
  ArrowRight: () => <div data-testid="arrow-icon" />,
  Clock: () => <div data-testid="clock-icon" />,
  WifiOff: () => <div data-testid="wifi-off-icon" />,
  RefreshCw: () => <div data-testid="refresh-icon" />,
}))

const mockSettings = { ra_user: 'testuser', ra_key: 'testkey' }

describe('AccountTab', () => {
  it('renders login form when no profile', () => {
    render(
      <AccountTab 
        settings={mockSettings} 
        setSettings={vi.fn()}
        isOnline={true}
        profile={null}
      />
    )
    expect(screen.getByPlaceholderText('Username')).toBeInTheDocument()
    expect(screen.getByText('SIGN IN')).toBeInTheDocument()
  })

  it('renders profile when connected', () => {
    const mockProfile = {
      User: 'tester',
      Rank: '1234',
      TotalPoints: '5000',
      Motto: 'Retro Master',
      RecentlyPlayed: []
    }
    render(
      <AccountTab 
        settings={mockSettings}
        profile={mockProfile}
        isOnline={true}
      />
    )
    expect(screen.getByText('TESTER')).toBeInTheDocument()
    expect(screen.getByText('#1234')).toBeInTheDocument()
    expect(screen.getByText('5000')).toBeInTheDocument()
  })

  it('renders offline mode when not online and no profile', () => {
    render(
      <AccountTab 
        settings={mockSettings}
        isOnline={false}
        profile={null}
      />
    )
    expect(screen.getByText('Offline Mode')).toBeInTheDocument()
    expect(screen.getByText('RETRY CONNECTION')).toBeInTheDocument()
  })

  it('calls handleLogout when SIGN OUT is clicked', () => {
    const setSettings = vi.fn()
    const mockProfile = { User: 'tester', RecentlyPlayed: [] }
    render(
      <AccountTab 
        settings={mockSettings}
        setSettings={setSettings}
        profile={mockProfile}
        isOnline={true}
      />
    )
    fireEvent.click(screen.getByText('SIGN OUT'))
    expect(setSettings).toHaveBeenCalled()
  })
})
