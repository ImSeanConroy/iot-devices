"use client";

import { useEffect, useState } from "react";
import { Badge } from "@/components/ui/badge";
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import io from "socket.io-client";

type LedColor = "red" | "green" | "blue" | "purple" | "pink" | "orange" | "yellow" | "off";

interface DeviceUpdatePayload {
  deviceId: string;
  data?: {
    state?: string;
    color?: string;
    [key: string]: unknown;
  };
}

interface MessageHistoryItem {
  id: string;
  deviceId: string;
  state: LedColor;
  sequence: string;
  receivedAt: string;
}

const socket = io("http://localhost:3000");

const toLedColor = (value: string | undefined): LedColor => {
  const normalized = value?.trim().toLowerCase();

  if (
    normalized === "red" ||
    normalized === "green" ||
    normalized === "blue" ||
    normalized === "purple" ||
    normalized === "pink" ||
    normalized === "orange" ||
    normalized === "yellow" ||
    normalized === "off"
  ) {
    return normalized;
  }

  return "off";
};

export default function LedStatusCard() {
  const [ledColor, setLedColor] = useState<LedColor>("off");
  const [messageHistory, setMessageHistory] = useState<MessageHistoryItem[]>([]);

  useEffect(() => {
    const handleDeviceUpdate = (payload: DeviceUpdatePayload) => {
      const nextColor = toLedColor(payload?.data?.state ?? payload?.data?.color);
      setLedColor(nextColor);

      const historyItem: MessageHistoryItem = {
        id: `${Date.now()}-${Math.random().toString(36).slice(2)}`,
        deviceId: payload?.deviceId ?? "unknown",
        state: nextColor,
        sequence: String(payload?.data?.sequence ?? "-"),
        receivedAt: new Date().toLocaleTimeString(),
      };

      setMessageHistory((previous) => [historyItem, ...previous].slice(0, 10));
    };

    socket.on("device-update", handleDeviceUpdate);

    return () => {
      socket.off("device-update", handleDeviceUpdate);
    };
  }, []);

  const badgeClass: Record<LedColor, string> = {
    red: "bg-red-500",
    green: "bg-green-500",
    blue: "bg-blue-500",
    purple: "bg-purple-500",
    pink: "bg-pink-500",
    orange: "bg-orange-500",
    yellow: "bg-yellow-400",
    off: "bg-gray-400",
  };

  const latestMessage = messageHistory[0];

  return (
    <div className="min-h-screen bg-accent flex items-center justify-center py-12">
      <div className="mx-auto w-full max-w-6xl px-4 py-8 sm:px-6 lg:px-8 pb-30">
        <div className="grid gap-6 lg:grid-cols-[340px_1fr]">
          <Card>
            <CardHeader>
              <CardTitle>Current LED Status</CardTitle>
              <CardDescription>Latest color from device-update</CardDescription>
            </CardHeader>
            <CardContent className="space-y-3">
              <div className="rounded-lg border p-4">
                <div className="flex items-center gap-3">
                  <Badge className={`${badgeClass[ledColor]} h-6 w-6 rounded-full border-0 p-0`} />
                  <span className="text-xl font-semibold capitalize">{ledColor}</span>
                </div>
              </div>

              <div className="grid grid-cols-2 gap-3 text-xs">
                <div className="rounded-lg border p-3">
                  <div>Messages cached</div>
                  <div className="mt-1 text-lg font-semibold">{messageHistory.length}</div>
                </div>
                <div className="rounded-lg border p-3">
                  <div>Last sequence</div>
                  <div className="mt-1 text-lg font-semibold">{latestMessage?.sequence ?? "-"}</div>
                </div>
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle>Last 10 Messages</CardTitle>
              <CardDescription>Most recent device-update events</CardDescription>
            </CardHeader>
            <CardContent>
              {messageHistory.length > 0 && (
                <ul className="max-h-115 space-y-2 overflow-y-auto pr-1 text-sm">
                  {messageHistory.map((item) => (
                    <li
                      key={item.id}
                      className="grid grid-cols-[1fr_auto] items-center gap-3 rounded-lg border px-3 py-2"
                    >
                      <div className="min-w-0">
                        <div className="flex items-center gap-2">
                          <Badge className={`${badgeClass[item.state]} h-3 w-3 rounded-full border-0 p-0`} />
                          <span className="font-medium capitalize">{item.state}</span>
                        </div>
                      </div>
                      <div className="text-right text-xs flex flex-row gap-4">
                        <div>seq: {item.sequence}</div>
                        <div>{item.receivedAt}</div>
                      </div>
                    </li>
                  ))}
                </ul>
              )}
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  );
}