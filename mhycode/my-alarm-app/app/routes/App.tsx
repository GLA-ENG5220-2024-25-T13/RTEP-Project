import React, { useState, useEffect } from 'react';
import { Bell, BellOff, Loader2, Zap, Waves, Circle, CircleDot, Cpu, MapPin, RadioTower } from 'lucide-react';
import { cn } from '~/lib/utils';
import { Toaster, toast } from 'sonner';
import { Switch } from "~/components/ui/switch"
import { Label } from "~/components/ui/label"

// 模拟 API，替换为您的实际 API 端点
const API_BASE_URL = '/api';

interface AlarmState {
    isAlarmActive: boolean;
    isManual: boolean;
    isSystem: boolean;
}

interface SensorState {
    power: boolean;
    cpuUsage: number;
    distances: { id: string; value: number | null; name: string }[];
    proximities: { id: string; state: 'normal' | 'alarm'; name: string }[];
}

const AlarmSystemDashboard = () => {
    const [alarmState, setAlarmState] = useState<AlarmState | null>(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [sensorState, setSensorState] = useState<SensorState | null>(null);
    const [systemTriggered, setSystemTriggered] = useState(false);
    const [isManualAlarmOn, setIsManualAlarmOn] = useState<'on' | 'off' | 'auto'>('auto'); // 初始状态为 'auto'，即默认自动报警

    // 初始状态
    const initialAlarmState: AlarmState = {
        isAlarmActive: false,
        isManual: false,
        isSystem: false,
    };

    const initialSensorState: SensorState = {
        power: true,
        cpuUsage: 10,
        distances: [],
        proximities: [],
    };

    // 使用 useEffect 模拟初始数据加载
    useEffect(() => {
        const fetchData = async () => {
            setLoading(true);
            try {
                // 模拟从后端获取传感器数据
                const sensorData = await fetchSensorData();
                setSensorState(sensorData);
                // 从后端获取初始报警状态
                const initialAlarmData = await fetchAlarmState();
                setAlarmState(initialAlarmData);
            } catch (err: any) {
                setError(err.message || 'Failed to fetch data');
                toast.error(`Error: ${err.message || 'Failed to fetch data'}`);
            } finally {
                setLoading(false);
            }
        };

        fetchData();
    }, []);

    // 模拟从后端获取传感器数据的函数
    const fetchSensorData = async (): Promise<SensorState> => {
        return new Promise((resolve) => {
            setTimeout(() => {
                const mockSensorData: SensorState = {
                    power: true,
                    cpuUsage: 15,
                    distances: [
                        { id: 'distance-1', value: 120, name: 'HC-SR04 超声波距离传感器' },
                        { id: 'distance-2', value: 25, name: 'GP2Y0A02YK0F 红外距离传感器' },
                    ],
                    proximities: [
                        { id: 'proximity-1', state: 'normal', name: 'PNP 常开型接近传感器' },
                        { id: 'proximity-2', state: 'alarm', name: 'E18-D80NK 光电接近传感器' },
                        { id: 'proximity-3', state: 'normal', name: 'LJ12A3-4-Z/BX 电感式接近传感器' },
                    ],
                };
                resolve(mockSensorData);
            }, 800);
        });
    };

    // 模拟从后端获取报警状态的函数
    const fetchAlarmState = async (): Promise<AlarmState> => {
        return new Promise((resolve) => {
            setTimeout(() => {
                // 模拟后端返回的初始报警状态
                const mockAlarmState: AlarmState = {
                    isAlarmActive: false,
                    isManual: false,
                    isSystem: false,
                };
                resolve(mockAlarmState);
            }, 500);
        });
    };

    // 模拟传感器数据变化和自动报警
    useEffect(() => {
        const interval = setInterval(() => {
            if (!sensorState) return;

            // 模拟传感器数据变化
            setSensorState(prevSensorState => {
                if (!prevSensorState) return initialSensorState;

                const newDistances = prevSensorState.distances.map(d => ({
                    ...d,
                    value: Math.max(0, Math.min(200, d.value! + (Math.random() - 0.5) * 20)),
                }));

                const newProximities = prevSensorState.proximities.map(p => ({
                    ...p,
                    state: Math.random() < 0.2 ? 'alarm' : 'normal',
                }));
                const newCpuUsage = Math.min(100, prevSensorState.cpuUsage + Math.random() * 10);

                // 检查是否应该自动触发警报，但不由前端直接设置报警状态
                let autoAlarmTriggered = false;
                for (const distance of newDistances) {
                    if (distance.value! < 50) {
                        autoAlarmTriggered = true;
                        break;
                    }
                }
                for (const proximity of newProximities) {
                    if (proximity.state === 'alarm') {
                        autoAlarmTriggered = true;
                        break;
                    }
                }

                // 只有在 'auto' 模式下才检查是否自动触发警报
                if (autoAlarmTriggered && isManualAlarmOn === 'auto') {
                    // 触发后端报警逻辑，这里只做模拟
                    triggerAlarm(true, false); // 传递 true 表示系统触发
                }

                return {
                    ...prevSensorState,
                    distances: newDistances,
                    proximities: newProximities,
                    power: Math.random() < 0.9,
                    cpuUsage: newCpuUsage,
                };
            });
        }, 2000);

        return () => clearInterval(interval);
    }, [isManualAlarmOn, sensorState]);

    // 切换报警状态
    const toggleAlarmState = async (mode: 'on' | 'off' | 'auto') => {
        setLoading(true);
        setIsManualAlarmOn(mode); // 更新前端状态，表示用户的选择

        try {
            // 调用后端 API 触发报警状态改变
            const newAlarmState = await triggerAlarm(mode === 'on', mode === 'off'); // 传递期望的状态
            setAlarmState(newAlarmState); // 更新报警状态
            if (mode === 'on') {
                toast.success('手动触发报警已开启');
            } else if (mode === 'off') {
                toast.success('手动触发报警已关闭');
            } else {
                toast.success('已切换到自动报警模式');
            }
        } catch (err: any) {
            setError(err.message || 'Failed to toggle alarm state');
            toast.error(`Error: ${err.message || 'Failed to toggle alarm state'}`);
        } finally {
            setLoading(false);
        }
    };

    // 模拟触发后端报警逻辑的函数
    const triggerAlarm = async (turnOn: boolean, turnOff: boolean): Promise<AlarmState> => {
        // 替换为实际的 API 调用
        return new Promise((resolve) => {
            setTimeout(() => {
                //  模拟后端处理报警逻辑
                let newAlarmState: AlarmState;
                if (turnOn) {
                    newAlarmState = { isAlarmActive: true, isManual: true, isSystem: false };
                    setSystemTriggered(false);
                } else if (turnOff) {
                    newAlarmState = { isAlarmActive: false, isManual: true, isSystem: false };
                    setSystemTriggered(false);
                }
                else {
                    // 自动模式，后端根据传感器数据决定是否激活警报
                    newAlarmState = { isAlarmActive: systemTriggered, isManual: false, isSystem: systemTriggered }; // 保持与 systemTriggered 一致
                }
                resolve(newAlarmState);
            }, 500);
        });
    };

    // 渲染传感器状态
    const renderSensor = (label: string, value: string | number | boolean | null, icon: React.ReactNode, colorClass: string) => {
        let displayValue = '';
        if (value === null) {
            displayValue = 'N/A';
        } else if (typeof value === 'boolean') {
            displayValue = value ? '正常' : '异常';
        } else if (typeof value === 'string') {
            displayValue = value === 'normal' ? '正常' : '报警';
        }
        else {
            displayValue = String(value);
        }

        return (
            <div className="flex items-center justify-between py-2 border-b border-gray-200 dark:border-gray-700 last:border-none">
                <div className="flex items-center gap-2">
                    {icon}
                    <span className="text-gray-700 dark:text-gray-300">{label}</span>
                </div>
                <span className={cn("font-medium", colorClass)}>
                    {displayValue}
                </span>
            </div>
        );
    };

    return (
        <div className="min-h-screen bg-gray-100 dark:bg-gray-900 flex items-center justify-center">
            <Toaster richColors />
            <div className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6 w-full max-w-md">
                <div className="flex items-center gap-4 mb-6">
                    {loading ? <Loader2 className="animate-spin w-6 h-6 text-gray-500" /> :
                        alarmState?.isAlarmActive ? <Bell className="w-6 h-6 text-red-500 animate-pulse" /> :
                            <BellOff className="w-6 h-6 text-green-500" />}
                    <h1 className={cn(
                        "text-2xl font-semibold",
                        loading ? "text-gray-500" :
                            alarmState?.isAlarmActive ? "text-red-600 dark:text-red-400" :
                                "text-green-600 dark:text-green-400"
                    )}>
                        {loading ? '正在加载报警状态...' :
                            alarmState?.isAlarmActive ? (alarmState.isManual ? '手动触发报警！' : alarmState.isSystem ? '系统自动报警！' : '警报已激活！') : '警报已关闭。'}
                    </h1>
                </div>
                <div className="mb-4">
                    <p className="text-gray-600 dark:text-gray-300">
                        当前状态:
                        <span className={cn(
                            "font-medium ml-1",
                            loading ? "text-gray-500" :
                                alarmState?.isAlarmActive ? "text-red-600 dark:text-red-400" : "text-green-600 dark:text-green-400"
                        )}>
                            {loading ? '加载中...' :
                                alarmState?.isAlarmActive ? (alarmState.isManual ? '已激活 (手动)' : alarmState.isSystem ? '已激活 (自动)' : '已激活') : '已关闭'}
                        </span>
                        {alarmState?.isAlarmActive && alarmState.isSystem && (
                            <span className="text-xs text-gray-500 dark:text-gray-400 ml-2">(系统自动触发，当前{isManualAlarmOn === 'off' ? '生效中' : '已忽略'})</span>
                        )}
                    </p>
                    {/* 添加 CPU 使用率显示 */}
                    {!loading && sensorState && (
                        <div className="mt-2">
                            <p className="text-gray-600 dark:text-gray-300">
                                CPU 使用率:
                                <span className={cn(
                                    "font-medium ml-1",
                                    sensorState.cpuUsage > 80 ? "text-red-500" : "text-green-500"
                                )}>
                                    {sensorState.cpuUsage}%
                                </span>
                            </p>
                        </div>
                    )}
                </div>

                {/* 传感器状态显示 */}
                <div className="mb-6">
                    <h2 className="text-lg font-semibold mb-4 text-gray-800 dark:text-gray-200">传感器状态</h2>
                    {loading ? (
                        <div className="flex items-center justify-center py-4">
                            <Loader2 className="animate-spin w-6 h-6 text-gray-500" />
                        </div>
                    ) : (
                        <div className="space-y-4">
                            {/* 距离传感器 */}
                            <div>
                                <h3 className="font-medium text-gray-700 dark:text-gray-300 mb-2 flex items-center gap-1.5">
                                    <MapPin className="w-4 h-4 text-blue-500" />
                                    距离传感器
                                </h3>
                                {sensorState?.distances.map(distance => (
                                    <div key={distance.id} className="ml-4">
                                        {renderSensor(
                                            distance.name,
                                            distance.value ?? null,
                                            <Waves className="w-4 h-4 text-blue-500" />,
                                            (distance.value ?? 500) < 50 ? 'text-red-500' : 'text-green-500'
                                        )}
                                    </div>
                                ))}
                            </div>

                            {/* 接近传感器 */}
                            <div>
                                <h3 className="font-medium text-gray-700 dark:text-gray-300 mb-2 flex items-center gap-1.5">
                                    <RadioTower className="w-4 h-4 text-yellow-500" />
                                    接近传感器
                                </h3>
                                {sensorState?.proximities.map(proximity => (
                                    <div key={proximity.id} className="ml-4">
                                        {renderSensor(
                                            proximity.name,
                                            proximity.state,
                                            proximity.state === 'alarm' ? <CircleDot className="w-4 h-4 text-red-500" /> : <Circle className="w-4 h-4 text-green-500" />,
                                            proximity.state === 'alarm' ? 'text-red-500' : 'text-green-500'
                                        )}
                                    </div>
                                ))}
                            </div>
                            {/* 电源状态 */}
                            <div>
                                <h3 className="font-medium text-gray-700 dark:text-gray-300 mb-2">
                                    <Zap className="w-4 h-4 text-yellow-500" />
                                    电源状态
                                </h3>
                                {renderSensor(
                                    '电源',
                                    sensorState?.power ?? true,
                                    <Zap className="w-4 h-4 text-yellow-500" />,
                                    sensorState?.power ? 'text-green-500' : 'text-red-500'
                                )}
                            </div>
                        </div>
                    )}
                </div>
                <div className="flex flex-col gap-4">
                    <div className="flex items-center justify-between">
                        <Label htmlFor="alarm-on" className="text-gray-700 dark:text-gray-300 flex items-center gap-2">
                            <Bell className="w-4 h-4 text-green-500" />
                            <span>强制开启警报</span>
                        </Label>
                        <Switch
                            id="alarm-on"
                            checked={isManualAlarmOn === 'on'}
                            onCheckedChange={(checked) => {
                                toggleAlarmState(checked ? 'on' : 'auto');
                            }}
                            className="data-[state=checked]:bg-green-500 data-[state=unchecked]:bg-gray-400"
                        />
                    </div>
                    <div className="flex items-center justify-between">
                        <Label htmlFor="alarm-off" className="text-gray-700 dark:text-gray-300 flex items-center gap-2">
                            <BellOff className="w-4 h-4 text-gray-500" />
                            <span>强制关闭警报</span>
                        </Label>
                        <Switch
                            id="alarm-off"
                            checked={isManualAlarmOn === 'off'}
                            onCheckedChange={(checked) => {
                                toggleAlarmState(checked ? 'off' : 'auto');
                            }}
                            className="data-[state=checked]:bg-gray-500 data-[state=unchecked]:bg-gray-400"
                        />
                    </div>
                </div>
            </div>
        </div>
    );
};

export default AlarmSystemDashboard;
