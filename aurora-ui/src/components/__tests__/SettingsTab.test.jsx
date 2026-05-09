import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi } from 'vitest'
import React from 'react'
import SettingsTab from '../SettingsTab'

// Mock dependencies
vi.mock('@tauri-apps/api/core', () => ({
  invoke: vi.fn(),
}))

vi.mock('lucide-react', () => ({
  Monitor: () => <div data-testid="monitor-icon" />,
  Trophy: () => <div data-testid="trophy-icon" />,
  Volume2: () => <div data-testid="volume-icon" />,
  Gamepad: () => <div data-testid="gamepad-icon" />,
  Keyboard: () => <div data-testid="keyboard-icon" />,
  ExternalLink: () => <div data-testid="external-icon" />,
  ShieldCheck: () => <div data-testid="shield-icon" />,
  Zap: () => <div data-testid="zap-icon" />,
}))

const mockSettings = {
  shader: 'none',
  volume: 80,
  ra_user: 'testuser',
  ra_key: 'testkey',
  p1_input: 'keyboard',
  p2_input: 'gamepad'
}

const mockControls = {
  p1: { up: 'ArrowUp', down: 'ArrowDown', left: 'ArrowLeft', right: 'ArrowRight', a: 'z', b: 'x', x: 'a', y: 's', select: 'Shift', start: 'Enter', l: 'q', r: 'w' },
  p2: { gp_up: 12, gp_down: 13, gp_left: 14, gp_right: 15, gp_a: 0, gp_b: 1, gp_x: 2, gp_y: 3, gp_select: 8, gp_start: 9, gp_l: 4, gp_r: 5 }
}

const mockKeyNames = { 'ArrowUp': 'UP', 'ArrowDown': 'DOWN', 'ArrowLeft': 'LEFT', 'ArrowRight': 'RIGHT', 'z': 'Z', 'x': 'X' }

describe('SettingsTab', () => {
  it('renders configuration title', () => {
    render(
      <SettingsTab 
        settings={mockSettings}
        controls={mockControls}
        keyNames={mockKeyNames}
      />
    )
    expect(screen.getByText('Configuration')).toBeInTheDocument()
  })

  it('changes shader when clicked', () => {
    const setSettings = vi.fn()
    render(
      <SettingsTab 
        settings={mockSettings}
        setSettings={setSettings}
        controls={mockControls}
        keyNames={mockKeyNames}
      />
    )
    fireEvent.click(screen.getByText('SCANLINES'))
    expect(setSettings).toHaveBeenCalledWith(expect.objectContaining({ shader: 'scanlines' }))
  })

  it('updates volume on range change', () => {
    const setSettings = vi.fn()
    render(
      <SettingsTab 
        settings={mockSettings}
        setSettings={setSettings}
        controls={mockControls}
        keyNames={mockKeyNames}
      />
    )
    const range = screen.getByRole('slider')
    fireEvent.change(range, { target: { value: '50' } })
    expect(setSettings).toHaveBeenCalledWith(expect.objectContaining({ volume: '50' }))
  })

  it('switches active player in mapping', () => {
    render(
      <SettingsTab 
        settings={mockSettings}
        controls={mockControls}
        keyNames={mockKeyNames}
      />
    )
    fireEvent.click(screen.getByText('PLAYER 2'))
    expect(screen.getByText('BTN 0')).toBeInTheDocument() // Button A for gamepad on P2
  })
})
