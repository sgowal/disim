function plot_recorded_data(file, type)
% file : filename where the data was recorded.
% type : f/k/v/s (flow, density, time mean speed or space mean speed)

data = load(file);
plot(data(:,1), data(:,2), 'k-');
if (type == 'f')
    ylabel('Flow [veh/h]');
elseif (type == 'k')
    ylabel('Density [veh/km]');
elseif (type == 's')
    clf;
    y = data(:,2)/2 + sqrt(data(:,2).^2/4 - data(:,3).^2);
    plot(data(:,1), y, 'k-');
    ylabel('Space Mean Speed [km/h]');
else
    % plot error bars
    hold on;
    errorbar(data(:,1), data(:,2), data(:,3), 'k.', 'MarkerSize', 0.1);
    hold off;
    ylabel('Time Mean Speed [km/h]');
end
xlabel('time [s]');

end